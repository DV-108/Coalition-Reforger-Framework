class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	// Spectator resource to use
	static const ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";
	
	// Time it takes for players to Init
	static const int PLAYER_INITILIZATION_TIME = 250;
	
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	[RplProp()]
	ref array<int> m_aDonators = {};
	
	[RplProp()]
	protected string m_sServerWorldTime;
	
	// Internal flag to prevent redundant replication updates
	protected bool m_bSuppressReplication = false;
	
	static ref CRF_GearScriptRolesConfig m_RolesConfig;
	
	protected CRF_SafestartManager m_SafestartManager;
	
	protected static CRF_GamemodeManager m_sInstance;
	
	// NEVER EVER SPAWN AN ENT WITH A PURE 0 WORLD VECTOR OR ELSE I WILL CASTRATE YOU I STG - Njpatman
	static const vector ZERO_SPAWN_VECTOR[4] = { "1 0 0", "0 1 0", "0 0 1", "0 0 0" };
	
	ref array<IEntity> m_aDeadBodies = {};
	protected ref array<IEntity> m_aForwardDeployZones = {};
	protected ref array<ref CRF_ForwardDeployRequest> m_aForwardDeployRequests = {};
	
	void CRF_GamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	static ResourceName GetSpectatorResource()
	{
		return SPECTATOR_RESOURCE;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_GearScriptRolesConfig RolesConfig()
	{
		return m_RolesConfig;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		// Initialize all required manager references
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		
		ResourceName rolesConfigPath;
		if (!CVON_VONGameModeComponent.GetInstance())
			  rolesConfigPath = "{4388548E9F600148}Configs/Gearscripts/CRF_Global_Roles_Config.conf";
		else
			rolesConfigPath = "{F04F02DBFC65553E}Configs/Gearscripts/Additional Configs/CRF_CVON_Global_Roles_Config.conf";
		
		m_RolesConfig = CRF_GearScriptRolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(rolesConfigPath).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	// SPECTATOR MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	* Check if a given entity is a spectator
	* @param entity Entity to check
	* @return True if entity is a spectator, false otherwise
	*/
	static bool IsSpectator(IEntity entity)
	{
		if (!entity)
			return false;
		
		return entity.GetPrefabData().GetPrefabName() == GetSpectatorResource();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if the local player is a spectator
	* @return True if local player is a spectator, false otherwise
	*/
	static bool IsSpectator()
	{
		IEntity mainEntity = SCR_PlayerController.GetLocalMainEntity();
		if (mainEntity && mainEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource())
			return true;
		
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (controlledEntity && controlledEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource())
			return true;

		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	// TIME MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	* Get the current server world time string
	* @return Formatted server time string
	*/
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Set the server world time
	* @param input Time string to set
	*/
	void SetServerWorldTime(string input)
	{
		SetServerWorldTimeSilent(input);
		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Set the server world time without triggering replication
	* @param input Time string to set
	*/
	protected void SetServerWorldTimeSilent(string input)
	{
		m_sServerWorldTime = input;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Update the server world time based on safestart time
	*/
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_SafestartManager.m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		SetServerWorldTimeSilent(SCR_FormatHelper.FormatTime(totalSeconds));
		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	/**
	* Update mission end timer and handle expiration
	*/
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_SafestartManager.m_iTimeMissionEnds - currentTime;
		int totalSeconds = (millis * 0.001);

		SetServerWorldTimeSilent(SCR_FormatHelper.FormatTime(totalSeconds));

		if (totalSeconds == 0) {
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			SetServerWorldTimeSilent("Mission Time Expired!");
		}

		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	// PLAYER JSON MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if local player is a moderator
	* @return True if local player is a moderator, false otherwise
	*/
	bool IsModerator()
	{
		return m_aModerators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsModerator(int playerId)
	{
		return m_aModerators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if local player is a donator
	* NOTE: Not used in game mode. Added for future uses.
	* @return True if local player is a donator, false otherwise
	*/
	bool IsDonator()
	{
		return m_aDonators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsDonator(int playerId)
	{
		return m_aDonators.Contains(playerId);
	}
	
	/**
	* Set a player status
	* @param playerId ID of the player to set as moderator or donator
	*/
	void SetPlayerStatus(int playerId, string role)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_aModerators.Contains(playerId) || m_aDonators.Contains(playerId))
			return;
		
		bool statusChanged = false;
		switch (role) {
			case "mod": {
				m_aModerators.Insert(playerId);
				statusChanged = true;
				break;
			}
			case "don": {
				m_aDonators.Insert(playerId);
				statusChanged = true;
				break;
			}
		}
			
		if (statusChanged && !m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	// FORWARD DEPLOY MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	//Needed so when we teleport players/vehicles the aren't spawning on top of each other.
	float m_fBuffer = 0;
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	    super.EOnFrame(owner, timeSlice);
	    m_fBuffer += timeSlice;
	    if (m_fBuffer > 0.1)
	    {
	        m_fBuffer = 0;
	        if (m_aForwardDeployRequests.Count() > 0)
	        {
	            CRF_ForwardDeployRequest request = m_aForwardDeployRequests.Get(0);
	            if (request)
	            {
	                PerformForwardDeploy(request.m_iPlayerId, request.m_vTransform);
	                m_aForwardDeployRequests.RemoveOrdered(0);
	            }
	        }
	        if (m_aForwardDeployRequests.Count() == 0)
	            ClearEventMask(owner, EntityEvent.FRAME);
	    }
	}
	
	//------------------------------------------------------------------------------------------------
	void AddForwardDeployZone(IEntity entity)
	{
		m_aForwardDeployZones.Insert(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	array<IEntity> GetForwardDeployZones()
	{
		return m_aForwardDeployZones;
	}
		
	//------------------------------------------------------------------------------------------------
	void DeleteAllForwardDeployZones()
	{
		foreach(IEntity zone : m_aForwardDeployZones)
		{
			if(zone)
				SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
		
		m_aForwardDeployZones.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateForwardDeployRequest(int playerId, vector transform)
	{
		ref CRF_ForwardDeployRequest request = new CRF_ForwardDeployRequest();
		request.m_iPlayerId = playerId;
		request.m_vTransform = transform;
		m_aForwardDeployRequests.Insert(request);
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	void PerformForwardDeploy(int playerId, vector transform)
	{
		vector initialSpawnLocation;
		SCR_WorldTools.FindEmptyTerrainPosition(initialSpawnLocation, transform, 10);
		vector params[4];
		params[3] = initialSpawnLocation;
		SCR_TerrainHelper.OrientToTerrain(params, GetGame().GetWorld(), true);
		vector finalSpawnLocation;
		SCR_TerrainHelper.SnapToGeometry(finalSpawnLocation, params[3], null);
		params[3] = finalSpawnLocation;
		SCR_Global.TeleportPlayer(playerId, finalSpawnLocation, SCR_EPlayerTeleportedReason.NONE);
		CRF_RplBroadcastManager.GetInstance().BroadcastVehiclePosUpdate(finalSpawnLocation, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	// BODY CLEANUP MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		
		m_aDeadBodies.Insert(instigatorContextData.GetVictimEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	void CleanUpBodies()
	{
		array<IEntity> bodiesToRemove = new array<IEntity>();
		bodiesToRemove.Reserve(m_aDeadBodies.Count()); // Pre-allocate capacity for performance
		
		foreach (IEntity body: m_aDeadBodies)
		{
			if (!body)
				continue;
			
			if (!GetGame().GetWorld().QueryEntitiesBySphere(body.GetOrigin(), 30, CleanUpBodyCallback, null))
				continue;

			bodiesToRemove.Insert(body);
		}
		
		int delay = 1;
		foreach (IEntity body: bodiesToRemove)
		{
			m_aDeadBodies.RemoveItem(body);
			//Lets not delete 100s of entities in one frame now
			GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 100 * delay, false, body);
			delay++;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	bool CleanUpBodyCallback(IEntity entity)
	{
		if (ChimeraCharacter.Cast(entity))
		{
			//Is this character dead
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
			if (damageManager)
			{
				if (damageManager.GetState() == EDamageState.DESTROYED)
					return true;
				else
					return false;
			}
		}
			
		return true;
	}
}

class CRF_ForwardDeployRequest
{
	int m_iPlayerId;
	vector m_vTransform;
}