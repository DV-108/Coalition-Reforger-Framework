//------------------------------------------------------------------------------------
// CRF_GamemodeClass: Base class definition for the Coalition Reforger Framework Gamemode
//------------------------------------------------------------------------------------
class CRF_GamemodeClass : SCR_BaseGameModeClass {}

//------------------------------------------------------------------------------------
// CRF_Gamemode: Main gamemode controller for Coalition Reforger Framework
// Handles mission flow, player management, respawn, and faction settings
//------------------------------------------------------------------------------------
class CRF_Gamemode : SCR_BaseGameMode
{
	//===================================================================================
	// ATTRIBUTES AND PROPERTIES
	//===================================================================================
	
	// Game State Properties
	//------------------------------------------------------------------------------------
	[RplProp(onRplName: "OnGamemodeStateChanged")]
	int m_GamemodeState = CRF_EGamemodeState.BRIEFING;

	[RplProp()]
	int m_SlottingState = CRF_ESlottingState.LEADERSANDMEDICS;
	
	// Attributes Set By Plugins
	//------------------------------------------------------------------------------------
	[Attribute("0", UIWidgets.Hidden)]
	bool m_bRespawnEnabled;

	[Attribute("0", UIWidgets.Hidden)]
	bool m_bWaveRespawn;

	[Attribute("60", UIWidgets.Hidden)]
	int m_iTimeToRespawn;
	
	[Attribute("45", UIWidgets.Hidden)]
	int m_iTimeLimitMinutes;
	
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bAllowEspionage;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bLockUnusedSlots;

	[Attribute("true", UIWidgets.Hidden)]
	bool m_bSafestartInstantlyEnabled;
	
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bUseSafestartTimeLimit;
	
	[Attribute("0", UIWidgets.Hidden)]
	int m_iSafestartTimeLimit;
	
	[Attribute("", UIWidgets.Hidden)]
	ref	array<ref CRF_MissionDescriptor> m_aMissionDescriptors;
	
	[Attribute("", UIWidgets.Hidden)]
	int m_iFactionOneRatio;

	[Attribute("", UIWidgets.Hidden)]
	int m_iFactionTwoRatio;
	
	[Attribute("", UIWidgets.Hidden)]
	string m_sFactionOneKey;

	[Attribute("", UIWidgets.Hidden)]
	string m_sFactionTwoKey;
	
	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_BluforSlots;

	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_OpforSlots;
	
	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_IndforSlots;
	
	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_CivSlots;
	
	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iBLUFORTickets;

	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iOPFORTickets;

	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iINDFORTickets;

	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iCIVTickets;
	
	// Advanced Gamemode Settings
	//------------------------------------------------------------------------------------
	[Attribute("0", "auto", "Disables AI Crouching", category: "CRF Gamemode Settings - Advanced")]
	bool m_bDisableAICrouching;
	
	[Attribute("true", "auto", "Disable chat messages except tickets & messages from admins/mods", category: "CRF Gamemode Settings - Advanced")]
	bool m_bDisableChat;

	// Gearscript Settings
	//------------------------------------------------------------------------------------
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_BLUFORGearScriptSettings;
	[RplProp()] ResourceName m_rBLUFORCurrentGearScript = m_BLUFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_OPFORGearScriptSettings;
	[RplProp()] ResourceName m_rOPFORCurrentGearScript = m_OPFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_INDFORGearScriptSettings;
	[RplProp()] ResourceName m_rINDFORCurrentGearScript = m_INDFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_CIVILIANGearScriptSettings;
	[RplProp()] ResourceName m_rCIVILIANCurrentGearScript = m_CIVILIANGearScriptSettings.m_rGearScript;
	
	// Manager References and System Components
	//------------------------------------------------------------------------------------
	protected ref ScriptInvoker m_OnStateChanged;
	protected static ref SCR_PlayerData m_PlayerData;
	
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_LoggingManager m_LoggingManager;
	
	protected static CRF_Gamemode m_sInstance;
	
	[RplProp()]
	protected vector m_vGenericSpawn;
	
	bool m_bIsInEndCredits = false;
	
	// Staggered Player Initialization System
	//------------------------------------------------------------------------------------
	protected ref array<int> m_aPendingPlayerInitializations = {};
	protected bool m_bProcessingInitializations = false;
	protected const int PLAYERS_PER_BATCH = 8;        // Players spawned per batch
	protected const int BATCH_INTERVAL_MS = 150;      // Milliseconds between batches
	protected float m_fBatchTimer = 0.0;              // Timer for batch processing

	//===================================================================================
	// STATIC METHODS
	//===================================================================================
	
	/**
	 * Returns the singleton instance of the CRF_Gamemode
	 * @return CRF_Gamemode instance or null if not available
	 */
	void CRF_Gamemode(IEntitySource src, IEntity parent)
	{
		m_sInstance = this;
		// Initialize ScriptInvoker to avoid null checks - PERFORMANCE OPTIMIZATION
		m_OnStateChanged = new ScriptInvoker();
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_Gamemode GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	vector GetGenericSpawn()
	{
		return m_vGenericSpawn;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnStateChanged()
	{
		return m_OnStateChanged;
	}

	//===================================================================================
	// INITIALIZATION AND SETUP
	//===================================================================================
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Initialize the gamemode and all required manager instances
	 * @param owner The entity that owns this component
	 */
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Load configs on dedicated server
		if (RplSession.Mode() == RplMode.Dedicated) {
			CRF_ModeratorConfig.LoadConfig();	
			CRF_DonatorConfig.LoadConfig();
			
			// Initialize sight arsenal registry for optimized RPC
			CRF_SightArsenalRegistry.InitializeRegistry();
		}
	
		// Initialize all manager references
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_LoggingManager = CRF_LoggingManager.GetInstance();
	}
	
	//===================================================================================
	// STATE MANAGEMENT
	//===================================================================================
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Progress to the next slotting state
	 * Updates all slotting UI and synchronizes across network
	 */
	void AdvanceSlottingState()
	{
		m_SlottingState += 1;
		Replication.BumpMe();  // m_SlottingState is [RplProp()] - auto-synced to clients
		
		// Notify all clients to refresh their slotting UI
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.NotifySlottingPhaseChanged();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Progress to the next gamemode state
	 * @param overriden Set to true to allow advancing from AAR or GAME states
	 */
	void AdvanceGamemodeState(bool overriden = false)
	{
		// Prevent advancing from AAR or GAME unless explicitly overridden
		if ((m_GamemodeState == CRF_EGamemodeState.AAR || m_GamemodeState == CRF_EGamemodeState.GAME) && !overriden)
			return;

		m_GamemodeState += 1;
		if (m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			foreach (Vehicle vehicle: CRF_VehicleGearscriptManager.GetInstance().GetSpawnedVehicleArray())
			{
				if (!vehicle)
					continue;
				
				vehicle.SpawnVehiclePassengers();
			}
		}
		Replication.BumpMe();
		OnGamemodeStateChanged();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Handle gamemode state changes
	 * Triggers UI updates and state-specific logic
	 */
	protected void OnGamemodeStateChanged()
	{
		// Server-side state change handling
		if (Replication.IsServer())
		{
			// Invoke state changed (invoker already initialized in constructor)
			m_OnStateChanged.Invoke();
			
			// Set basic game mode states for basegamemode
			// useful for default components that reference it like datacollector
			switch (m_GamemodeState) {
				case CRF_EGamemodeState.GAME: {
					SetGameState(SCR_EGameModeState.GAME);
					break;
				}
				
				case CRF_EGamemodeState.AAR: {
					//SetGameState(SCR_EGameModeState.POSTGAME);
					SCR_DataCollectorComponent dataCollector = GetGame().GetDataCollector();
					dataCollector.OnGameModeEnd(GetEndGameData());
					
					array<int> players = {};
					GetGame().GetPlayerManager().GetAllPlayers(players);
					
					foreach (int player : players)
					{
						// Skip disconnected players
						if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
							continue;
						
						// Process player statistics data
						ProcessStats(dataCollector, player);
					}
					
					CRF_RplBroadcastManager.GetInstance().BroadcastOutro();
					
					// Stores player profiles who havent disconnected
					dataCollector.OnGameEnd();
					
					// Make sure we close logging memory leak
					m_LoggingManager.OnGameModeEnd(GetEndGameData());
					break;
				}
				
			}	
		}
		
		CRF_PlayerMenuManager playerMenuManager = CRF_PlayerMenuManager.GetInstance();
		if (playerMenuManager)
			playerMenuManager.OpenCurrentStateMenu();
	}
	
	//------------------------------------------------------------------------------------------------
	void ProcessStats(SCR_DataCollectorComponent dataCollector, int player)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(player);
		//PrintFormat("[CRF] Logging Stats for player %1",name);
		// Process player statistics data
		if (!m_PlayerData)
		{
			if (!dataCollector)
			{
				Print("[CRF] CRF_Gamemode SCR_DataCollectorComponent: No data collector was found.", LogLevel.ERROR);
				return;
			}
	
			m_PlayerData = dataCollector.GetPlayerData(player, false);
	
			// If player data isn't available yet, register for notification when it arrives
			if (!m_PlayerData)
			{
				SCR_DataCollectorCommunicationComponent communicationComponent = SCR_DataCollectorCommunicationComponent.Cast(
					GetGame().GetPlayerManager().GetPlayerController(player).FindComponent(SCR_DataCollectorCommunicationComponent)
				);
				
				if (communicationComponent)
					communicationComponent.GetOnDataReceived().Insert(OnDataReceived);
			} else {
				m_PlayerData.CalculateStatsChange();
			}
		}
	}
	
	//===================================================================================
	// PLAYER MANAGEMENT
	//===================================================================================
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Handle player data received from network
	 * @param playerData Player statistics and progress data
	 */
	protected void OnDataReceived(SCR_PlayerData playerData)
	{
		m_PlayerData = playerData;
		m_PlayerData.CalculateStatsChange();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Process player connection after authentication
	 * @param iPlayerID ID of the connecting player
	 */
	protected override void OnPlayerAuditSuccess(int iPlayerID)
	{
		super.OnPlayerAuditSuccess(iPlayerID);
		
		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;

		// Check if player is the mission designer and grant admin chat
		string playerName = GetGame().GetPlayerManager().GetPlayerName(iPlayerID);
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		
		if (missionHeader && missionHeader.m_sAuthor && !missionHeader.m_sAuthor.IsEmpty())
		{
			string authorName = missionHeader.m_sAuthor;
			if (playerName.ToLower() == authorName.ToLower())
			{
				// Grant session admin (admin chat) to mission designer
				GetGame().GetPlayerManager().GivePlayerRole(iPlayerID, EPlayerRole.SESSION_ADMINISTRATOR);
			}
		}

		// Check if player is a moderator/donator and set privileges
		string playerIdentity = GetGame().GetBackendApi().GetPlayerIdentityId(iPlayerID);
		if (!playerIdentity.IsEmpty()) {
			if (CRF_ModeratorConfig.IsModerator(playerIdentity))
				m_GamemodeManager.SetPlayerStatus(iPlayerID, "mod");
			
			if (CRF_DonatorConfig.IsDonator(playerIdentity))
				m_GamemodeManager.SetPlayerStatus(iPlayerID, "don");
		}
	}
	
	
	//------------------------------------------------------------------------------------------------
	/*!
		Called after a player is disconnected.
		\param playerId PlayerId of disconnected player.
	*/
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		
		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually. 
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}
		
		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);
	}
	
	//===================================================================================
	// ENTITY MANAGEMENT
	//===================================================================================
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Process entity spawning for players
	 * @param entity The spawned entity
	 */
	protected override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		
		if (!GetGame().InPlayMode() || !character || ! character.GetPrefabData() || !CRF_RoleHelper.IsValidGearscriptResource(character.GetPrefabData().GetPrefabName()))
			return;
		
		// Schedule gearscript identity setup with appropriate delay
		GetGame().GetCallqueue().Call(
			m_GearscriptManager.SetEntityIdentity, 
			character
		);
	
		// Apply gearscript if not on client
		if (RplSession.Mode() != RplMode.Client)
		{
			// Ensure gearscript manager is available
			if (!m_GearscriptManager)
				m_GearscriptManager = CRF_GearscriptManager.GetInstance();
			
			// Schedule gear setup with appropriate delay
			GetGame().GetCallqueue().Call(
				m_GearscriptManager.SetEntityGear, 
				character, 
				character.GetPrefabData().GetPrefabName()
			);
		};
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Process entity death/destruction for players
	 * Handles respawn and spectator logic
	 * @param entity The destroyed entity
	 * @param killerEntity The entity that caused the destruction
	 * @param instigator The instigator context
	 */
	protected override void OnControllableDestroyed(IEntity entity, IEntity killerEntity, notnull Instigator instigator)
	{
		super.OnControllableDestroyed(entity, killerEntity, instigator);

		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Note: The base game's data collector is automatically triggered by super.OnControllableDestroyed()
		// Our modded CRF_SCR_DataCollectorComponent.OnPlayerKilled() hooks into this and calls the logging manager
		
		// Create instigator context for tracking kill details
		SCR_InstigatorContextData instigatorContextData = new SCR_InstigatorContextData(-1, entity, killerEntity, instigator);
		int playerId = instigatorContextData.GetVictimPlayerID();
		
		// Return if not a player character
		if (playerId <= 0 || instigatorContextData.GetVictimCharacterControlType() == SCR_ECharacterControlType.POSSESSED_AI)
			return;

		// Determine delay time for respawn/spectator
		int delay = 2000;
		if (CRF_GamemodeManager.IsSpectator(entity))
			delay = 0;
		
		// Get player faction
		Faction faction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
		FactionKey factionKey;
		
		if (faction)
			factionKey = faction.GetFactionKey();

		// Handle respawn if enabled and tickets available
		if (m_RespawnManager.m_bCurrentRespawnEnabled && 
			!CRF_GamemodeManager.IsSpectator(entity) && 
			m_GamemodeState != CRF_EGamemodeState.AAR && 
			m_RespawnManager.TicketsRemaining(factionKey) &&
			!m_RespawnManager.GetFactionSpawnpoints(factionKey).IsEmpty() &&
			!factionKey.IsEmpty())
		{
			// Deduct ticket
			m_RespawnManager.SubtractTicket(factionKey, 1);

			// Display respawn screen
			GetGame().GetCallqueue().CallLater(
				m_RplBroadcastManager.SendRespawnScreen,
				(delay + 150),
				false,
				playerId
			);
		}
		
		// Update slot death state so player gets put into spec
		int slotID = m_SlottingManager.GetCharacterSlotID(entity);
		
		if(slotID != -1)
			m_SlottingManager.UpdateSlotDeathState(slotID, true);

		// Get death position for spectator camera initialization
		vector deathPosition[4];
		entity.GetWorldTransform(deathPosition);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateGearscriptResource(string factionKey, string resource)
	{
		switch (factionKey)
		{
			case "BLUFOR" : m_rBLUFORCurrentGearScript = resource; break;
			case "OPFOR" : m_rOPFORCurrentGearScript = resource; break;
			case "INDFOR" : m_rINDFORCurrentGearScript = resource; break;
			case "CIV" : m_rCIVILIANCurrentGearScript = resource; break;
		}
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateGenericSpawn()
	{
	 	m_vGenericSpawn = CRF_MissionHelper.GetAOCenter();
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	bool DoesFactionShareMarker(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": 
				return m_BLUFORGearScriptSettings.m_bEnableShareableMarkers;
			case "OPFOR": 
				return m_OPFORGearScriptSettings.m_bEnableShareableMarkers;
			case "INDFOR": 
				return m_INDFORGearScriptSettings.m_bEnableShareableMarkers;
			case "CIV": 
				return m_CIVILIANGearScriptSettings.m_bEnableShareableMarkers;
    		 }
    		return true;
 	}
	
	//------------------------------------------------------------------------------------------------
	bool IsSideBFTEnabled(string factionKey)
	{
		switch(factionKey)
		{
			case "BLUFOR":
				return m_BLUFORGearScriptSettings.m_bEnableBFT;
			case "OPFOR":
				return m_OPFORGearScriptSettings.m_bEnableBFT;
			case "INDFOR":
				return m_INDFORGearScriptSettings.m_bEnableBFT;
			case "CIV":
				return m_CIVILIANGearScriptSettings.m_bEnableBFT;
		}
   		return true;
	}
}

modded class SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	void SetGameState(SCR_EGameModeState state)
	{
		m_eGameState = state;
		Replication.BumpMe();
	}
}

