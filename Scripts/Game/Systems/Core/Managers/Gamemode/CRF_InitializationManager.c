class CRF_InitializationManagerClass : ScriptComponentClass
{
}

class CRF_InitializationManager : ScriptComponent
{
	protected static CRF_InitializationManager m_sInstance;
	void CRF_InitializationManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_InitializationManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Initialize a player into the game either as a playable character or spectator
	* @param playerId ID of the player to initialize
	* @param spawnLocation Location to spawn the player (Use "CRF_GamemodeManager.ZERO_SPAWN_VECTOR" as the input to have players spawn at their original slot location)
	*/
	void InitilizePlayer(int playerId)
	{
		if (!IsValidSpawnVector(spawnLocation[3]) && spawnLocation != ZERO_SPAWN_VECTOR)
		{
			Print(string.Format("[CRF ERROR]: %1 DOESN'T HAVE VALID SPAWN VECTOR!", playerId), LogLevel.ERROR);
			return;
		};
		
		if (playerId <= 0)
			return;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
			
		SCR_ChimeraCharacter playerCharacter = null;
		Faction faction = null;
		bool alreadyCreated;
		
		// Determine if player should be spectator or playable character
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			// SPECTATOR PATH: Create initial entity for spectators
			playerCharacter = CreateSpectatorEntity(CRF_GamemodeManager.ZERO_SPAWN_VECTOR);
	
			
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			
			RemovePlayerFromCurrentGroup(playerId);
			DisableDamageForSpectator(playerCharacter);
		} 
		else 
		{
			// PLAYABLE CHARACTER PATH: Skip initial entity, spawn real character directly
			// This optimization eliminates 50% of entity spawns (no temporary initial entities)
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnLocation, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			
			// If character already existed (respawn case), clean up any old initial/spectator entity
			if (alreadyCreated)
			{
				DeleteOldInitialEntity(playerController, playerCharacter);
			}
			
			CRF_MenuManager.GetInstance().RemovePlayerFromAnyChannel(playerId, false);
		}
		
		if (playerCharacter)
		{
			AssignFactionToPlayer(playerController, faction);
			GetGame().GetCallqueue().CallLater(InitilizePlayerCharacter, CRF_GamemodeManager.PLAYER_INITILIZATION_TIME, false, playerId, playerController, playerCharacter);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Assign the player to the set entity
	* @param playerId ID of the player
	* @param playerController controller of the player
	* @param playerCharacter entity the player will take
	*/
	protected void InitilizePlayerCharacter(int playerId, SCR_PlayerController playerController, SCR_ChimeraCharacter playerCharacter)
	{
		// Validate that player is still connected before proceeding
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;
			
		// Validate that the character still exists
		if (!playerCharacter)
			return;
			
		AssignCharacterToPlayer(playerController, playerCharacter);
		
		// Wait a frame for the entity assignment to take effect, then verify success
		GetGame().GetCallqueue().Call(VerifyCharacterAssignment, playerId, playerController, playerCharacter);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Verify that character assignment was successful and complete initialization
	* @param playerId ID of the player
	* @param playerController controller of the player
	* @param playerCharacter entity the player should control
	*/
	protected void VerifyCharacterAssignment(int playerId, SCR_PlayerController playerController, SCR_ChimeraCharacter playerCharacter)
	{
		// Validate that player is still connected
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;
			
		// Check if character assignment was successful
		IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		
		// If player is still controlling the initial entity, retry the assignment
		if (controlledEntity && controlledEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource() && (playerCharacter.GetPrefabData().GetPrefabName() != GetSpectatorResource()))
		{
			// Force reassign the character
			AssignCharacterToPlayer(playerController, playerCharacter);
			
			// Schedule another verification attempt
			GetGame().GetCallqueue().CallLater(VerifyCharacterAssignment, 100, false, playerId, playerController, playerCharacter);
			return;
		}
		
		// Assignment successful, complete initialization
		if (playerCharacter.GetPrefabData().GetPrefabName() != GetSpectatorResource())
			AssignPlayerToGroup(playerId);
		
		RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));

		GetGame().GetCallqueue().CallLater(CRF_RplBroadcastManager.GetInstance().InitilizePlayerBroadcast, PLAYER_INITILIZATION_TIME, false, playerId, playerRplComp.Id());
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Remove player from their current group if any
	* @param playerId ID of the player to remove from group
	*/
	protected void RemovePlayerFromCurrentGroup(int playerId)
	{
		SCR_AIGroup currentGroup = m_GroupsManagerComponent.GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Disable damage handling for spectator character
	* @param character Character to disable damage for
	*/
	protected void DisableDamageForSpectator(SCR_ChimeraCharacter character)
	{
		if (!character)
			return;
			
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent)); 
		if (damManager)
			damManager.EnableDamageHandling(false);
	}

	//------------------------------------------------------------------------------------------------
	/**
	* Assign faction to player controller
	* @param playerController Player controller to assign faction to
	* @param faction Faction to assign
	*/
	protected void AssignFactionToPlayer(SCR_PlayerController playerController, Faction faction)
	{
		if (!faction || !playerController)
			return;
			
		SCR_PlayerFactionAffiliationComponent affiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)
		);
		
		if (affiliationComponent)
			affiliationComponent.RequestFaction(faction);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Assign character entity to player controller
	* @param playerController Player controller to assign character to
	* @param character Character to assign
	*/
	protected void AssignCharacterToPlayer(SCR_PlayerController playerController, SCR_ChimeraCharacter character)
	{
		if (!character || !playerController)
			return;
		
		// Delete the old initial entity BEFORE assigning new character
		// This prevents "ghost" entities
		DeleteOldInitialEntity(playerController, character);
		
		playerController.SetInitialMainEntity(character);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Delete old initial entity if it exists (prevents ghost entities)
	* @param playerController Player controller to check
	* @param newCharacter The new character being assigned (don't delete this one)
	*/
	static void DeleteOldInitialEntity(SCR_PlayerController playerController, IEntity newCharacter)
	{
		if (!playerController)
			return;
			
		IEntity oldEntity = playerController.GetMainEntity();
		if (!oldEntity || oldEntity == newCharacter)
			return;
		
		// Check if old entity is an initial entity (spawned at 1000m)
		string oldPrefab = oldEntity.GetPrefabData().GetPrefabName();
		if (oldPrefab == SPECTATOR_RESOURCE || oldPrefab.Contains("InitialEntity"))
		{
			// Log deletion for debugging
			Print(string.Format("[CRF] Deleting ghost initial entity for player %1 at position %2", 
				playerController.GetPlayerId(), 
				oldEntity.GetOrigin()), 
				LogLevel.VERBOSE);
			
			// Delete immediately to prevent replication
			SCR_EntityHelper.DeleteEntityAndChildren(oldEntity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Assign player to their slotted group
	* @param playerId ID of the player to assign
	*/
	protected void AssignPlayerToGroup(int playerId)
	{
		SCR_AIGroup group = m_SlottingManager.GetPlayerSlotGroup(playerId);
		if (!group)
			return;
			
		int groupId = group.GetGroupID();
		if (groupId == -1)
			return;
			
		m_GroupsManagerComponent.AddPlayerToGroup(groupId, playerId);
		
		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (groupComponent)
			groupComponent.RequestJoinGroup(groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Create a spectator entity in the world
	* @return The created spectator character
	*/
	protected SCR_ChimeraCharacter CreateSpectatorEntity(vector spawnLocation[4])
	{
		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = spawnLocation;
		
		Resource spectatorRes = Resource.Load(GetSpectatorResource());
		return SCR_ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(spectatorRes, GetGame().GetWorld(), spawnParams));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Get existing character or create a new one for playable roles
	* @param playerId ID of the player
	* @param overrideLocation Optional spawn location
	* @return The character entity
	*/
	protected SCR_ChimeraCharacter GetOrCreatePlayableCharacter(int playerId, vector overrideLocation[4], out bool alreadyCreated)
	{
		alreadyCreated = true;
		SCR_ChimeraCharacter playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
		
		if (!playerCharacter || playerCharacter.GetCharacterController().IsDead())
		{
			alreadyCreated = false;
			
			CRF_RplBroadcastManager.GetInstance().SendCharacterLoadingScreen(playerId);
			playerCharacter = SpawnPlayableEntity(playerId, overrideLocation);
			
			if (!playerCharacter)
			{
				Print(string.Format("[CRF_GamemodeManager] ERROR: Failed to spawn character for player %1", playerId), LogLevel.ERROR);
				return null;
			}
			
			// Notify data collector about the spawn
			SCR_DataCollectorComponent dc = GetGame().GetDataCollector();
			if (dc)
			{
				// Use our custom notification method since we don't use the spawn request system
				dc.NotifyPlayerSpawned(playerId, playerCharacter);
			}
		}
		
		return playerCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_ChimeraCharacter SpawnPlayableEntity(int playerId, vector overrideLocation[4])
	{
		int slotId = GetPlayerSlotID(playerId);
		if (slotId < 0)
			return null;
			
		ResourceName resourceName = GetPlayerSlotResource(playerId);
		if (resourceName.IsEmpty())
			return null;
		
		vector playerSlotVector[4];
		CRF_RespawnManager.GetInstance().FindInitalSpawnLocation(GetPlayerSlotFaction(playerId).GetFactionKey(), GetPlayerSlotGroup(playerId), playerSlotVector);

		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		if (overrideLocation[3] != vector.Zero)
		{	
			foreach (int i, vector vec : overrideLocation)
			{
				if (overrideLocation[i] == vector.Zero)
					overrideLocation[i] = playerSlotVector[i];
			}
			
			spawnParams.Transform[3] = overrideLocation[3];
		} else {
			spawnParams.Transform = playerSlotVector;
		}

		spawnParams.Transform[3][1] + spawnParams.Transform[3][1] + 0.5; //Go up 1 incase theres some weird slope, floor issue
		vector surface;
		SCR_TerrainHelper.SnapToGeometry(surface, spawnParams.Transform[3], {}, GetGame().GetWorld());
		spawnParams.Transform[3] = surface;
		SCR_TerrainHelper.OrientToTerrain(spawnParams.Transform);
		
		GetSafeSpawnTransform(spawnParams.Transform, 12, spawnParams.Transform);
		
		// Spawn the character using cached resource
		Resource resource = GetCachedResource(resourceName);
		if (!resource)
		{
			Print(string.Format("[CRF_SlottingManager] Failed to load resource: %1", resourceName), LogLevel.ERROR);
			return null;
		}
		
		SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(
			GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams)
		);
		
		if (!playerCharacter)
			return null;
		
		// Update character faction
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(playerCharacter.FindComponent(FactionAffiliationComponent));
		facComp.SetAffiliatedFaction(GetPlayerSlotFaction(playerId));
	
		// Update slot data
		RplComponent charRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (charRplComp)
			UpdateSlotCharacter(slotId, charRplComp.Id());
		
		return playerCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	void GetSafeSpawnTransform(vector baseTransform[4], float radius, out vector trasnformOut[4])
	{
		// Base Enfusion spawn already handles position validation
		// Simply apply a small random offset for player spacing during mass spawns
		vector outTransform[4] = baseTransform;
		
		// Add random offset to prevent exact position overlap
		float angle = Math.RandomFloat01() * Math.PI2;
		float dist = Math.RandomFloat01() * radius;
		vector offset = Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist);
		
		outTransform[3] = baseTransform[3] + offset;
		
		// Snap to terrain geometry
		vector surface;
		SCR_TerrainHelper.SnapToGeometry(surface, outTransform[3], {}, GetGame().GetWorld());
		if (surface != vector.Zero)
		{
			outTransform[3] = surface;
			SCR_TerrainHelper.OrientToTerrain(outTransform);
		}
		
		trasnformOut = outTransform;
	}
}