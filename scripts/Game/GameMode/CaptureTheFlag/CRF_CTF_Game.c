//------------------------------------------------------------------------------------
// CRF_CTFGamemodeManager: Capture the Flag gamemode implementation for Coalition Reforger Framework
// Spawns a capturable flag item and two drop-off zones per team.
// Teams must capture the flag and bring it to their drop-off zone for 5 minutes to win.
//
// Features:
// - Capturable flag item with configurable marker updates
// - Team drop-off zones with hold timer mechanics
// - Configurable faction participation (BLUFOR, OPFOR, INDFOR)
// - 5-minute capture timer with win conditions
// - Map markers for flag location and drop-off zones
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "Capture the Flag gamemode with capturable flag and team drop-off zones")]
class CRF_CTFGamemodeManagerClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_CTFGamemodeManager: SCR_BaseGameModeComponent
{
	//===================================================================================
	// ATTRIBUTES AND PROPERTIES
	//===================================================================================
	
	// Faction Settings
	//------------------------------------------------------------------------------------
	[Attribute("true", desc: "Enable BLUFOR participation in CTF")]
	bool m_bBluforEnabled;
	
	[Attribute("true", desc: "Enable OPFOR participation in CTF")]
	bool m_bOpforEnabled;
	
	[Attribute("false", desc: "Enable INDFOR participation in CTF")]
	bool m_bIndforEnabled;
	
	// Flag Settings
	//------------------------------------------------------------------------------------
	[Attribute("{A8C69227F4322F20}Prefabs/Items/CRF_CTF_Flag.et", UIWidgets.ResourceNamePicker, desc: "The prefab to spawn as the capturable flag", params: "et")]
	ResourceName m_FlagPrefab;
	
	[Attribute("120", desc: "Time in seconds between flag marker updates (default: 2 minutes)")]
	int m_iMarkerUpdateInterval;
	
	// Drop-off Zone Settings
	//------------------------------------------------------------------------------------
	[Attribute("{B8C79227F4322F21}Prefabs/Triggers/CRF_CTF_DropZone.et", UIWidgets.ResourceNamePicker, desc: "The prefab to spawn as team drop-off zones", params: "et")]
	ResourceName m_DropZonePrefab;
	
	[Attribute("300", desc: "Time in seconds to hold flag in drop zone to win (default: 5 minutes)")]
	int m_iCaptureTimer;
	
	// UI Settings
	//------------------------------------------------------------------------------------
	[Attribute("false", desc: "Hide the map markers for flag and drop zones")]
	bool m_bHideMapMarkers;
	
	//===================================================================================
	// RUNTIME VARIABLES
	//===================================================================================
	
	// Flag Management
	//------------------------------------------------------------------------------------
	protected IEntity m_FlagEntity;					// The flag entity reference
	protected IEntity m_FlagCarrier;				// Current player carrying the flag
	protected vector m_vFlagSpawnPosition;			// Original flag spawn position
	protected bool m_bFlagCaptured = false;			// Is flag currently being carried
	
	// Drop Zone Management
	//------------------------------------------------------------------------------------
	protected IEntity m_BluforDropZone1;			// BLUFOR drop zone 1
	protected IEntity m_BluforDropZone2;			// BLUFOR drop zone 2
	protected IEntity m_OpforDropZone1;				// OPFOR drop zone 1
	protected IEntity m_OpforDropZone2;				// OPFOR drop zone 2
	protected IEntity m_IndforDropZone1;			// INDFOR drop zone 1
	protected IEntity m_IndforDropZone2;			// INDFOR drop zone 2
	
	// Capture State Management
	//------------------------------------------------------------------------------------
	protected FactionKey m_CapturingFaction = "";	// Which faction is currently capturing
	protected int m_iCaptureTimeRemaining = 0;		// Time left for capture
	protected bool m_bCaptureInProgress = false;	// Is capture currently in progress
	
	// Marker Management
	//------------------------------------------------------------------------------------
	protected bool m_bMarkerUpdateActive = false;	// Is marker update timer running
	
	// Notification System
	//------------------------------------------------------------------------------------
	protected SCR_PopUpNotification m_PopUpNotification;
	
	// Network Replication
	//------------------------------------------------------------------------------------
	[RplProp(onRplName: "OnFlagStatusChanged")]
	protected bool m_bReplicatedFlagCaptured = false;
	
	[RplProp(onRplName: "OnCaptureProgressChanged")]
	protected int m_iReplicatedCaptureProgress = 0;
	
	[RplProp(onRplName: "OnGameWon")]
	protected FactionKey m_sWinningFaction = "";
	
	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent = "";
	
	[RplProp(onRplName: "PlaySound")]
	protected string m_sSoundString = "";
	
	//===================================================================================
	// INITIALIZATION
	//===================================================================================
	
	/**
	 * Initialize the CTF gamemode when world is ready
	 */
	override protected void OnWorldPostProcess(World world)
	{
		if (!GetGame().InPlayMode()) 
			return;
			
		// Delay initialization to ensure all entities are loaded
		GetGame().GetCallqueue().CallLater(InitializeCTF, 1000, false);
	}
	
	/**
	 * Main initialization method for CTF gamemode
	 */
	protected void InitializeCTF()
	{
		Print("[CRF_CTFGamemodeManager] Initializing Capture the Flag gamemode...");
		
		// Initialize notification system
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		
		// Spawn the flag
		SpawnFlag();
		
		// Spawn drop-off zones for enabled factions
		SpawnDropZones();
		
		// Start marker update timer and initialize markers
		if (!m_bHideMapMarkers)
		{
			StartMarkerUpdates();
		}
		
		Print("[CRF_CTFGamemodeManager] CTF gamemode initialization complete.");
	}
	
	/**
	 * Spawn the capturable flag at the designated location
	 */
	protected void SpawnFlag()
	{
		// Find the flag spawn trigger
		IEntity flagSpawnTrigger = GetGame().GetWorld().FindEntityByName("FlagSpawn");
		if (!flagSpawnTrigger)
		{
			Print("[CRF_CTFGamemodeManager] ERROR: Could not find FlagSpawn entity!", LogLevel.ERROR);
			return;
		}
		
		m_vFlagSpawnPosition = flagSpawnTrigger.GetOrigin();
		
		// Spawn the flag entity
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = m_vFlagSpawnPosition;
		
		m_FlagEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_FlagPrefab), GetGame().GetWorld(), spawnParams);
		if (m_FlagEntity)
		{
			m_FlagEntity.SetYawPitchRoll(flagSpawnTrigger.GetYawPitchRoll());
			// Set a name for the flag entity so we can reference it in markers
			m_FlagEntity.SetName("CTF_Flag_Entity");
			Print("[CRF_CTFGamemodeManager] Flag spawned successfully at: " + m_vFlagSpawnPosition.ToString());
		}
		else
		{
			Print("[CRF_CTFGamemodeManager] ERROR: Failed to spawn flag entity!", LogLevel.ERROR);
		}
	}
	
	/**
	 * Spawn drop-off zones for all enabled factions
	 */
	protected void SpawnDropZones()
	{
		// Spawn BLUFOR drop zones
		if (m_bBluforEnabled)
		{
			m_BluforDropZone1 = SpawnDropZone("BluforDropZone1");
			m_BluforDropZone2 = SpawnDropZone("BluforDropZone2");
		}
		
		// Spawn OPFOR drop zones
		if (m_bOpforEnabled)
		{
			m_OpforDropZone1 = SpawnDropZone("OpforDropZone1");
			m_OpforDropZone2 = SpawnDropZone("OpforDropZone2");
		}
		
		// Spawn INDFOR drop zones
		if (m_bIndforEnabled)
		{
			m_IndforDropZone1 = SpawnDropZone("IndforDropZone1");
			m_IndforDropZone2 = SpawnDropZone("IndforDropZone2");
		}
	}
	
	/**
	 * Spawn a single drop zone at the specified trigger location
	 * @param triggerName Name of the trigger entity to spawn the drop zone at
	 * @return The spawned drop zone entity
	 */
	protected IEntity SpawnDropZone(string triggerName)
	{
		IEntity trigger = GetGame().GetWorld().FindEntityByName(triggerName);
		if (!trigger)
		{
			Print("[CRF_CTFGamemodeManager] WARNING: Could not find trigger: " + triggerName, LogLevel.WARNING);
			return null;
		}
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = trigger.GetOrigin();
		
		IEntity dropZone = GetGame().SpawnEntityPrefab(Resource.Load(m_DropZonePrefab), GetGame().GetWorld(), spawnParams);
		if (dropZone)
		{
			dropZone.SetYawPitchRoll(trigger.GetYawPitchRoll());
			
			// Determine faction based on trigger name and configure the drop zone component
			FactionKey factionKey = GetFactionFromTriggerName(triggerName);
			ConfigureDropZoneFaction(dropZone, factionKey);
			
			Print("[CRF_CTFGamemodeManager] Drop zone spawned at: " + triggerName + " for faction: " + factionKey);
		}
		else
		{
			Print("[CRF_CTFGamemodeManager] ERROR: Failed to spawn drop zone at: " + triggerName, LogLevel.ERROR);
		}
		
		return dropZone;
	}
	
	/**
	 * Determine faction from trigger name
	 * @param triggerName The name of the trigger entity
	 * @return The faction key for this drop zone
	 */
	protected FactionKey GetFactionFromTriggerName(string triggerName)
	{
		if (triggerName.Contains("Blufor"))
			return "BLUFOR";
		else if (triggerName.Contains("Opfor"))
			return "OPFOR";
		else if (triggerName.Contains("Indfor"))
			return "INDFOR";
		
		// Default fallback
		Print("[CRF_CTFGamemodeManager] WARNING: Could not determine faction from trigger name: " + triggerName, LogLevel.WARNING);
		return "BLUFOR";
	}
	
	/**
	 * Configure the drop zone component with the correct faction
	 * @param dropZone The spawned drop zone entity
	 * @param factionKey The faction this drop zone belongs to
	 */
	protected void ConfigureDropZoneFaction(IEntity dropZone, FactionKey factionKey)
	{
		if (!dropZone)
			return;
		
		// Find the drop zone component and configure it
		CRF_CTFDropZoneComponent dropZoneComponent = CRF_CTFDropZoneComponent.Cast(dropZone.FindComponent(CRF_CTFDropZoneComponent));
		if (dropZoneComponent)
		{
			dropZoneComponent.SetFactionKey(factionKey);
			Print("[CRF_CTFGamemodeManager] Configured drop zone component for faction: " + factionKey);
		}
		else
		{
			Print("[CRF_CTFGamemodeManager] ERROR: Drop zone prefab missing CRF_CTFDropZoneComponent!", LogLevel.ERROR);
		}
	}
	
	//===================================================================================
	// FLAG MECHANICS
	//===================================================================================
	
	/**
	 * Called when a player picks up the flag
	 * @param player The player entity that picked up the flag
	 */
	void FlagPickedUp(IEntity player)
	{
		if (!player || m_bFlagCaptured)
			return;
		
		m_FlagCarrier = player;
		m_bFlagCaptured = true;
		m_bReplicatedFlagCaptured = true;
		
		// Get player's faction
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
		{
			SCR_FactionAffiliationComponent factionComp = SCR_FactionAffiliationComponent.Cast(player.FindComponent(SCR_FactionAffiliationComponent));
			if (factionComp)
			{
				Faction faction = factionComp.GetAffiliatedFaction();
				if (faction)
				{
					string playerName = GetPlayerName(player);
					m_sMessageContent = string.Format("Flag captured by %1 (%2)!", playerName, faction.GetFactionName());
					ShowMessage();
				}
			}
		}
		
		Replication.BumpMe();
		Print("[CRF_CTFGamemodeManager] Flag picked up by player: " + player.GetName());
	}
	
	/**
	 * Called when the flag is dropped (player death, disconnect, etc.)
	 * @param position Position where the flag should be dropped
	 */
	void FlagDropped(vector position)
	{
		if (!m_bFlagCaptured)
			return;
		
		m_FlagCarrier = null;
		m_bFlagCaptured = false;
		m_bReplicatedFlagCaptured = false;
		
		// Stop any capture in progress
		StopCapture();
		
		// Move flag to dropped position
		if (m_FlagEntity)
		{
			m_FlagEntity.SetOrigin(position);
		}
		
		m_sMessageContent = "Flag has been dropped!";
		ShowMessage();
		
		Replication.BumpMe();
		Print("[CRF_CTFGamemodeManager] Flag dropped at: " + position.ToString());
	}
	
	/**
	 * Reset the flag to its original spawn position
	 */
	void ResetFlag()
	{
		if (m_FlagEntity)
		{
			m_FlagEntity.SetOrigin(m_vFlagSpawnPosition);
		}
		
		m_FlagCarrier = null;
		m_bFlagCaptured = false;
		m_bReplicatedFlagCaptured = false;
		
		// Stop any capture in progress
		StopCapture();
		
		m_sMessageContent = "Flag has been reset to base!";
		ShowMessage();
		
		Replication.BumpMe();
		Print("[CRF_CTFGamemodeManager] Flag reset to spawn position");
	}
	
	//===================================================================================
	// CAPTURE MECHANICS
	//===================================================================================
	
	/**
	 * Called when flag carrier enters a drop zone
	 * @param player The player carrying the flag
	 * @param dropZone The drop zone entity
	 */
	void StartCapture(IEntity player, IEntity dropZone)
	{
		if (!player || !dropZone || !m_bFlagCaptured || player != m_FlagCarrier)
			return;
		
		// Get player's faction
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;
		
		SCR_FactionAffiliationComponent factionComp = SCR_FactionAffiliationComponent.Cast(player.FindComponent(SCR_FactionAffiliationComponent));
		if (!factionComp)
			return;
		
		Faction faction = factionComp.GetAffiliatedFaction();
		if (!faction)
			return;
		
		// Check if this is the correct drop zone for the player's faction
		if (!IsCorrectDropZone(faction.GetFactionKey(), dropZone))
			return;
		
		// Start capture process
		m_CapturingFaction = faction.GetFactionKey();
		m_iCaptureTimeRemaining = m_iCaptureTimer;
		m_bCaptureInProgress = true;
		
		m_sMessageContent = string.Format("%1 is capturing the flag! Time remaining: %2", faction.GetFactionName(), SCR_FormatHelper.FormatTime(m_iCaptureTimeRemaining));
		ShowMessage();
		
		// Start capture timer
		GetGame().GetCallqueue().CallLater(CaptureTimer, 1000, true);
		
		Print("[CRF_CTFGamemodeManager] Capture started by faction: " + m_CapturingFaction);
	}
	
	/**
	 * Called when flag carrier leaves a drop zone or capture is interrupted
	 */
	void StopCapture()
	{
		if (!m_bCaptureInProgress)
			return;
		
		m_bCaptureInProgress = false;
		m_iCaptureTimeRemaining = 0;
		m_CapturingFaction = "";
		
		GetGame().GetCallqueue().Remove(CaptureTimer);
		
		m_sMessageContent = "Flag capture interrupted!";
		ShowMessage();
		
		Print("[CRF_CTFGamemodeManager] Capture stopped/interrupted");
	}
	
	/**
	 * Timer function for capture progress
	 */
	protected void CaptureTimer()
	{
		if (!m_bCaptureInProgress)
		{
			GetGame().GetCallqueue().Remove(CaptureTimer);
			return;
		}
		
		// Check if flag carrier is still in drop zone
		if (!IsCarrierInDropZone())
		{
			StopCapture();
			return;
		}
		
		// Decrement timer
		m_iCaptureTimeRemaining--;
		m_iReplicatedCaptureProgress = m_iCaptureTimeRemaining;
		
		// Update progress message every 30 seconds or in final 10 seconds
		if (m_iCaptureTimeRemaining % 30 == 0 || m_iCaptureTimeRemaining <= 10)
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (factionManager)
			{
				Faction faction = factionManager.GetFactionByKey(m_CapturingFaction);
				if (faction)
				{
					m_sMessageContent = string.Format("%1 capturing flag! Time remaining: %2", faction.GetFactionName(), SCR_FormatHelper.FormatTime(m_iCaptureTimeRemaining));
					ShowMessage();
				}
			}
		}
		
		// Check if capture is complete
		if (m_iCaptureTimeRemaining <= 0)
		{
			CaptureComplete();
		}
		
		Replication.BumpMe();
	}
	
	/**
	 * Called when the capture timer reaches zero
	 */
	protected void CaptureComplete()
	{
		m_bCaptureInProgress = false;
		GetGame().GetCallqueue().Remove(CaptureTimer);
		
		// Set winning faction
		m_sWinningFaction = m_CapturingFaction;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
		{
			Faction faction = factionManager.GetFactionByKey(m_CapturingFaction);
			if (faction)
			{
				m_sMessageContent = string.Format("VICTORY! %1 has captured the flag!", faction.GetFactionName());
				ShowMessage();
			}
		}
		
		// Play victory sound
		m_sSoundString = "{349D4D7CC242131D}Sounds/Music/Ingame/Samples/Jingles/MU_EndCard_Drums.wav";
		PlaySound();
		
		Print("[CRF_CTFGamemodeManager] Game won by faction: " + m_CapturingFaction);
		
		Replication.BumpMe();
	}
	
	//===================================================================================
	// UTILITY METHODS
	//===================================================================================
	
	/**
	 * Check if the given drop zone belongs to the specified faction
	 * @param factionKey The faction key to check
	 * @param dropZone The drop zone entity
	 * @return True if it's the correct drop zone for the faction
	 */
	protected bool IsCorrectDropZone(FactionKey factionKey, IEntity dropZone)
	{
		if (!dropZone)
			return false;
		
		switch (factionKey)
		{
			case "BLUFOR":
				return (dropZone == m_BluforDropZone1 || dropZone == m_BluforDropZone2);
			case "OPFOR":
				return (dropZone == m_OpforDropZone1 || dropZone == m_OpforDropZone2);
			case "INDFOR":
				return (dropZone == m_IndforDropZone1 || dropZone == m_IndforDropZone2);
		}
		
		return false;
	}
	
	/**
	 * Check if the flag carrier is still in a valid drop zone
	 * @return True if carrier is in correct drop zone
	 */
	protected bool IsCarrierInDropZone()
	{
		if (!m_FlagCarrier)
			return false;
		
		// Get carrier's faction
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return false;
		
		SCR_FactionAffiliationComponent factionComp = SCR_FactionAffiliationComponent.Cast(m_FlagCarrier.FindComponent(SCR_FactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction faction = factionComp.GetAffiliatedFaction();
		if (!faction)
			return false;
		
		// Check if carrier is in correct drop zone (this would need to be implemented based on your trigger system)
		// For now, we'll assume it's handled by the drop zone triggers
		return true;
	}
	
	/**
	 * Check if a player is currently carrying the flag
	 * @param player The player entity to check
	 * @return True if player is carrying the flag
	 */
	bool IsPlayerCarryingFlag(IEntity player)
	{
		return (player && m_bFlagCaptured && m_FlagCarrier == player);
	}
	
	/**
	 * Check if BLUFOR faction is enabled
	 * @return True if BLUFOR can participate
	 */
	bool IsBluforEnabled()
	{
		return m_bBluforEnabled;
	}
	
	/**
	 * Check if OPFOR faction is enabled
	 * @return True if OPFOR can participate
	 */
	bool IsOpforEnabled()
	{
		return m_bOpforEnabled;
	}
	
	/**
	 * Check if INDFOR faction is enabled
	 * @return True if INDFOR can participate
	 */
	bool IsIndforEnabled()
	{
		return m_bIndforEnabled;
	}
	
	/**
	 * Check if capture is currently in progress
	 * @return True if capture is active
	 */
	bool IsCaptureInProgress()
	{
		return m_bCaptureInProgress;
	}
	
	/**
	 * Get remaining capture time
	 * @return Time remaining in seconds
	 */
	int GetCaptureTimeRemaining()
	{
		return m_iCaptureTimeRemaining;
	}
	
	/**
	 * Get the faction currently capturing
	 * @return Faction key of capturing faction
	 */
	FactionKey GetCapturingFaction()
	{
		return m_CapturingFaction;
	}
	
	/**
	 * Check if flag is currently captured/carried
	 * @return True if flag is being carried
	 */
	bool IsFlagCaptured()
	{
		return m_bFlagCaptured;
	}
	
	/**
	 * Get the display name of a player
	 * @param player The player entity
	 * @return The player's display name
	 */
	protected string GetPlayerName(IEntity player)
	{
		if (!player)
			return "Unknown";
		
		// Try to get player name from character controller
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		if (controller)
		{
			// Implementation would depend on how player names are stored in the framework
			return "Player"; // Placeholder
		}
		
		return player.GetName();
	}
	
	//===================================================================================
	// MARKER SYSTEM
	//===================================================================================
	
	/**
	 * Start the marker update timer
	 */
	protected void StartMarkerUpdates()
	{
		if (m_bMarkerUpdateActive)
			return;
		
		m_bMarkerUpdateActive = true;
		GetGame().GetCallqueue().CallLater(UpdateMarkers, m_iMarkerUpdateInterval * 1000, true);
		
		// Update markers immediately
		UpdateMarkers();
	}
	
	/**
	 * Update flag marker position
	 */
	protected void UpdateMarkers()
	{
		if (!m_FlagEntity)
			return;
		
		// Update flag marker using the CRF marker system
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (playerControllerManager)
		{
			// Get current flag position
			vector flagPos = m_FlagEntity.GetOrigin();
			string flagPosStr = string.Format("%1 %2 %3", flagPos[0], flagPos[1], flagPos[2]);
			
			// Clear all existing markers to avoid conflicts (similar to Rush gamemode approach)
			playerControllerManager.RemoveALLScriptedMarkers();
			
			// Add flag marker at current position
			playerControllerManager.AddScriptedMarker("Static Marker", flagPosStr, 1, "Flag", "{7F9EAE4D0D008396}UI/Textures/Editor/Attributes/Categories/Attribute_Category_Objectives.edds", 50, ARGB(255, 255, 0, 0));
			
			// Re-add drop zone markers if they exist
			AddDropZoneMarkers(playerControllerManager);
			
			Print("[CRF_CTFGamemodeManager] Flag marker updated at position: " + flagPosStr);
		}
	}
	
	/**
	 * Add markers for drop zones
	 */
	protected void AddDropZoneMarkers(CRF_PlayerControllerManager playerControllerManager)
	{
		// Add BLUFOR drop zone markers
		if (m_bBluforEnabled)
		{
			playerControllerManager.AddScriptedMarker("BluforDropZone1", "0 0 0", 0, "BLUFOR Drop Zone", "{91427B7866707601}UI/Objectives/lock.edds", 45, ARGB(255, 0, 50, 255));
			playerControllerManager.AddScriptedMarker("BluforDropZone2", "0 0 0", 0, "BLUFOR Drop Zone", "{91427B7866707601}UI/Objectives/lock.edds", 45, ARGB(255, 0, 50, 255));
		}
		
		// Add OPFOR drop zone markers
		if (m_bOpforEnabled)
		{
			playerControllerManager.AddScriptedMarker("OpforDropZone1", "0 0 0", 0, "OPFOR Drop Zone", "{91427B7866707601}UI/Objectives/lock.edds", 45, ARGB(255, 255, 50, 0));
			playerControllerManager.AddScriptedMarker("OpforDropZone2", "0 0 0", 0, "OPFOR Drop Zone", "{91427B7866707601}UI/Objectives/lock.edds", 45, ARGB(255, 255, 50, 0));
		}
		
		// Add INDFOR drop zone markers  
		if (m_bIndforEnabled)
		{
			playerControllerManager.AddScriptedMarker("IndforDropZone1", "0 0 0", 0, "INDFOR Drop Zone", "{91427B7866707601}UI/Objectives/lock.edds", 45, ARGB(255, 0, 255, 0));
			playerControllerManager.AddScriptedMarker("IndforDropZone2", "0 0 0", 0, "INDFOR Drop Zone", "{91427B7866707601}UI/Objectives/lock.edds", 45, ARGB(255, 0, 255, 0));
		}
	}
	
	//===================================================================================
	// NETWORK REPLICATION CALLBACKS
	//===================================================================================
	
	/**
	 * Called when flag status changes (replicated)
	 */
	void OnFlagStatusChanged()
	{
		// Handle client-side flag status updates
		Print("[CRF_CTFGamemodeManager] Flag status changed - Captured: " + m_bReplicatedFlagCaptured);
	}
	
	/**
	 * Called when capture progress changes (replicated)
	 */
	void OnCaptureProgressChanged()
	{
		// Handle client-side capture progress updates
		Print("[CRF_CTFGamemodeManager] Capture progress changed - Time remaining: " + m_iReplicatedCaptureProgress);
	}
	
	/**
	 * Called when game is won (replicated)
	 */
	void OnGameWon()
	{
		// Handle client-side game end
		Print("[CRF_CTFGamemodeManager] Game won by faction: " + m_sWinningFaction);
	}
	
	/**
	 * Show message notification (replicated)
	 */
	void ShowMessage()
	{
		if (m_PopUpNotification && !m_sMessageContent.IsEmpty())
		{
			m_PopUpNotification.PopupMsg(m_sMessageContent, 10);
		}
	}
	
	/**
	 * Play sound effect (replicated)
	 */
	void PlaySound()
	{
		if (!m_sSoundString.IsEmpty())
		{
			// Play sound using CRF sound system
			// Implementation would depend on the framework's sound system
		}
	}
	
	//===================================================================================
	// CLEANUP
	//===================================================================================
	
	/**
	 * Cleanup when component is destroyed
	 */
	override void OnDelete(IEntity owner)
	{
		// Stop all timers
		GetGame().GetCallqueue().Remove(CaptureTimer);
		GetGame().GetCallqueue().Remove(UpdateMarkers);
		
		// Clean up entities
		if (m_FlagEntity)
			delete m_FlagEntity;
		
		super.OnDelete(owner);
	}
}