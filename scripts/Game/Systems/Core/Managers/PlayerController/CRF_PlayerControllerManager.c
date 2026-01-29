/*
 * Coalition Reforger Framework (CRF) Player Controller Component
 * 
 * This component manages player-specific functionality including:
 * - Camera and spectator controls
 * - UI and menu management
 * - Game state handling
 * - Admin commands and messaging
 * - Radio setup and audio controls
 * - Map markers and gameplay indicators
 */
[ComponentEditorProps(category: "Player Controller Components", description: "")]
class CRF_PlayerControllerManagerClass : ScriptComponentClass
{

}

class CRF_PlayerControllerManager : ScriptComponent
{
	// UI and Display
	string m_sHintText = "Type Here";      // Text displayed for hints to player
	bool m_bHUDVisible = true;             // Controls visibility of HUD elements
	Widget m_wSavedHintWidget;             // Reference to hint widget for reuse
	
	// Game Performance Settings
	protected int m_iFPS = -1;              // Stored user FPS setting (-1 means uninitialized)
	protected int m_iAudioSetting = -1;     // Stored audio volume (-1 means uninitialized)
	
	// Game Systems
	protected CRF_Gamemode m_Gamemode;                      // Reference to the active gamemode
	protected CRF_GamemodeManager m_GamemodeManager;        // Reference to the gamemode manager
	protected CRF_SlottingManager m_SlottingManager;		 // Reference to the slotting manager
	protected CRF_RplToAuthorityManager m_RplToAuthorityManager;  // Network authority manager
	protected CRF_PlayerCameraManager m_CameraManager;                  // Reference to the local camera manager
	
	// Map and Markers
	ref array<string> m_aScriptedMarkers = {};  // Custom map markers
	
	protected static CRF_PlayerControllerManager m_sInstance;

	//------------------------------------------------------------------------------------------------
	// STATIC ACCESSORS
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the instance of this component from the player controller
	 * @return CRF_PlayerControllerManager - The player controller component instance or null if unavailable
	 */
	
	void CRF_PlayerControllerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_PlayerControllerManager GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------------------------------------------
	// INITIALIZATION
	//------------------------------------------------------------------------------------------------

	/**
	 * Initializes the player controller component
	 * Sets up input listeners and schedules initial setup calls
	 */
	void InitilizePlayerControllerComp()
	{
		// Skip initialization on dedicated servers or in editor
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Get references to required systems
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
		m_CameraManager = CRF_PlayerCameraManager.GetInstance();

		// Register input action handlers
		GetGame().GetInputManager().AddActionListener("CRF_ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
		GetGame().GetInputManager().AddActionListener("CRF_AdminForceReady", EActionTrigger.DOWN, AdminForceReady);
		GetGame().GetInputManager().AddActionListener("CRF_OpenLobby", EActionTrigger.PRESSED, OpenSlottingMenu);
		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
		
		GetGame().GetCallqueue().Call(AddMsgAction);
		GetGame().GetCallqueue().Call(InitFPSLock);
		GetGame().GetCallqueue().Call(InitAudioLock);
		GetGame().GetCallqueue().Call(OpenCurrentStateMenu);
	}
	
	//------------------------------------------------------------------------------------------------
	// HUD AND CAMERA CONTROLS
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Toggles the visibility of the HUD
	 */
	void UpdateHUDVisible()
	{
		m_bHUDVisible = !m_bHUDVisible;
	}

	/**
	 * Updates entity position and resets physics
	 * @param position - New position/transform
	 */
	void UpdateEntityPos(vector position[4])
	{
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();

		// Align to terrain if not a character
		if (!ChimeraCharacter.Cast(player))
			SCR_TerrainHelper.OrientToTerrain(position);

		// Teleport or transform entity
		BaseGameEntity baseGameEntity = BaseGameEntity.Cast(player);
		if (baseGameEntity)
			baseGameEntity.Teleport(position);
		else
			player.SetWorldTransform(position);

		// Reset physics to prevent unwanted movement
		Physics phys = player.GetPhysics();
		if (phys)
		{
			phys.SetVelocity(vector.Zero);
			phys.SetAngularVelocity(vector.Zero);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// GAME SETTINGS MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Initializes audio lock by storing current volume and setting to 0
	 */
	void InitAudioLock()
	{
		m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
		SetSFXVolume(0);
	}
	
	/**
	 * Sets SFX volume to specified level
	 * @param volume - Volume level to set
	 */
	void SetSFXVolume(int volume)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, volume);
	}
	
	/**
	 * Sets FPS limit to specified value
	 * @param video - Video settings container
	 * @param fps - FPS limit to set
	 */
	void SetFPS(BaseContainer video, int fps)
	{
		video.Set("MaxFps", fps);
		GetGame().UserSettingsChanged();
	}
	
	/**
	 * Retrieves and stores initial user FPS setting
	 * @param video - Video settings container
	 */
	void GetInitialUserFPSValue(BaseContainer video)
	{
		video.Get("MaxFps", m_iFPS);
	}
	
	/**
	 * Initializes FPS lock by storing current value and setting to 30
	 */
	void InitFPSLock()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		//GetInitialUserFPSValue(video);
		SetFPS(video, 30);
	}
	
	/**
	 * Restores user settings to original values
	 */
	void ResetSettingsToStoredValues()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		
		// Restore FPS if initialized
		SetFPS(video, 0);
		
		// Restore audio if initialized
		SetSFXVolume(100);
	}
	
	//------------------------------------------------------------------------------------------------
	// MENU MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Opens the slotting menu for player assignment
	 * @param value - Input value (1.0 for pressed)
	 * @param reason - Trigger reason
	 */
	void OpenSlottingMenu(float value = 0.0, EActionTrigger reason = 0)
	{
		if (value != 1)
			return;

		// Check if appropriate menu is already open
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
		{
			if (topMenu.IsInherited(CRF_PreviewMenu) || topMenu.IsInherited(CRF_SlottingMenu))
				return;
			else if (topMenu.IsInherited(CRF_SpectatorMenu))
				GetGame().GetMenuManager().CloseMenu(topMenu);
		}

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
	}	

	//------------------------------------------------------------------------------------------------
	// ADMIN MESSAGING SYSTEM
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Registers chat commands for admin messaging
	 */
	void AddMsgAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("admin");
		invoker.Insert(SendAdminMessage);
		
		ChatCommandInvoker invoker2 = chatPanelManager.GetCommandInvoker("a");
		invoker2.Insert(SendAdminMessage);
		
		ChatCommandInvoker invoker3 = chatPanelManager.GetCommandInvoker("r");
		invoker3.Insert(ReplyAdminMessage);
		
		ChatCommandInvoker invoker4 = chatPanelManager.GetCommandInvoker("reply");
		invoker4.Insert(ReplyAdminMessage);
		
		ChatCommandInvoker invoker5 = chatPanelManager.GetCommandInvoker("aar");
		invoker5.Insert(Advance_Callback);
		
		ChatCommandInvoker invoker6 = chatPanelManager.GetCommandInvoker("save");
		invoker6.Insert(SaveMission_Callback);
	}
	
	/**
	 * Sends an admin message from the player to server
	 * @param panel - Chat panel
	 * @param data - Message content
	 */
	void SendAdminMessage(SCR_ChatPanel panel, string data)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		
		if (!data.Length() > 0)
		{
			chatComponent.ShowMessage("You need to include your message after /a");
			return;
		}	
		
		chatComponent.ShowMessage(string.Format("Message Sent: \"%1\"", data));
		m_RplToAuthorityManager.SendAdminMessage(data, playerID);
	}

	/**
	 * Allows admins to reply to specific players
	 * @param panel - Chat panel
	 * @param data - Message including player ID and content
	 */
	void ReplyAdminMessage(SCR_ChatPanel panel, string data)
	{
		// Verify sender is admin or moderator
		if (!SCR_Global.IsAdmin() && !m_GamemodeManager.IsModerator())
			return;
		
		// Parse player ID and message from input
		array<string> dataSplit = {};
		data.Split(" ", dataSplit, false);
		int playerId;
		string toSend;
		
		for (int i = 0; i < dataSplit.Count(); i++)
		{
			if (dataSplit[i] == "0")
			{
				dataSplit.RemoveOrdered(i);
				playerId = 0;
				toSend = SCR_StringHelper.Join(" ", dataSplit, true);
				break;
			}

			if (dataSplit[i].ToInt() > 0)
			{
				playerId = dataSplit[i].ToInt();
				dataSplit.RemoveOrdered(i);
				toSend = SCR_StringHelper.Join(" ", dataSplit, true);
				break;
			}
		}
		
		// Get chat component
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		// Validate player ID
		if (!playerId)
		{
			chatComponent.ShowMessage("INVALID PLAYER ID");
			return;
		}
		
		if (!GetGame().GetPlayerManager().GetPlayerName(playerId))
		{
			chatComponent.ShowMessage("INVALID PLAYER ID");
			return;
		}
		
		// Get the ID of the admin replying to the ticket
		int adminID = SCR_PlayerController.GetLocalPlayerId();

		// Send message
		chatComponent.ShowMessage(string.Format("Message Sent to %2: \"%1\"", 
			toSend, 
			GetGame().GetPlayerManager().GetPlayerName(playerId)));
		
		toSend = string.Format("\"%1\"", toSend);
		m_RplToAuthorityManager.ReplyAdminMessage(toSend, playerId, adminID, true);
	}

	//------------------------------------------------------------------------------------------------
	// GAMEMODE CONTROLS
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Toggles ready state for player's side/faction
	 * Only faction leaders can toggle ready state
	 */
	void ToggleSideReady()
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;

		SCR_AIGroup playersGroup = groupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		if (!playersGroup)
			return;

		string playerName = GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId());
		if (!playerName || playerName == "")
			return;

		// Only group leaders can toggle ready state
		if (playersGroup.IsPlayerLeader(SCR_PlayerController.GetLocalPlayerId()))
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		
			if (faction.GetFactionKey() == "")
				return;
			
			m_RplToAuthorityManager.ToggleSideReady(faction.GetFactionKey(), playerName, false);
		}
	}

	/**
	 * Admin command to force ready state for all sides
	 */
	void AdminForceReady()
	{
		if (!SCR_Global.IsAdmin())
			return;
		
		m_RplToAuthorityManager.ToggleSideReady("", 
			GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId()), 
			true);
	}
	
	/**
	 * Teleports a player to another player's location
	 * @param playerId1 - Player to teleport
	 * @param playerId2 - Destination player
	 */
	void TeleportLocalPlayer(int playerId1, int playerId2)
	{
		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 10);
		spawnParams.Transform[3] = teleportLocation;

		SCR_Global.TeleportPlayer(playerId1, teleportLocation);
	}
	
	/**
	 * Callback for advancing gamemode state with optional faction winner parameter
	 * Usage: /aar [faction]
	 * Examples: /aar, /aar blufor, /aar blu, /aar opfor, /aar opf, /aar indfor, /aar ind, /aar civ
	 */
	void Advance_Callback(SCR_ChatPanel panel, string data)
	{
		// Check if admin privileges are required
		if (!SCR_Global.IsAdmin())
		{
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("You need admin privileges to use the /aar command.");
			}
			return;
		}
		
		// Parse faction parameter if provided
		if (data && data.Length() > 0)
		{
			// Clean up the input - remove extra spaces and convert to uppercase
			data.Trim();
			data.ToUpper();
			
			// Map short faction names to full names
			FactionKey winningFaction = "";
			switch (data)
			{
				case "BLUFOR":
				case "BLU":
				case "B":
				case "BLUE":
					winningFaction = "BLUFOR";
					break;
					
				case "OPFOR":
				case "OPF":
				case "O":
				case "RED":
					winningFaction = "OPFOR";
					break;
					
				case "INDFOR":
				case "IND":
				case "I":
				case "INDEPENDENT":
				case "GREEN":
					winningFaction = "INDFOR";
					break;
					
				case "CIV":
				case "C":
				case "CIVILIAN":
					winningFaction = "CIV";
					break;
					
				default:
					// Invalid faction specified
					if (panel)
					{
						SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
						if (chatComponent)
						{
							string validOptions = "Valid faction options: blufor (blu), opfor (opf), indfor (ind), civ";
							chatComponent.ShowMessage(string.Format("Invalid faction '%1'. %2", data, validOptions));
						}
					}
					return;
			}
			
			// Set the winning faction in the logging manager
			CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
			if (loggingManager)
			{
				loggingManager.SetWinningFaction(winningFaction, "manual");
				
				// Show confirmation message
				if (panel)
				{
					SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
					if (chatComponent)
						chatComponent.ShowMessage(string.Format("Winner set to %1. Advancing to AAR...", winningFaction));
				}
			}

			// Advance to AAR state
			m_RplToAuthorityManager.RequestAdvanceGamemodeState(true);
		}
		else
		{
			// No faction specified - show current usage
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("Usage: /aar [faction] - Examples: /aar blufor, /aar opfor, /aar indfor, /aar civ");
					
			}
			return;
		}
	}
	
	/**
	 * Callback for triggering a manual mission save
	 * Usage: /save [save name]
	 * Examples: /save, /save After Attack, /save Checkpoint 1
	 */
	void SaveMission_Callback(SCR_ChatPanel panel, string data)
	{
		// Check if admin privileges are required
		if (!SCR_Global.IsAdmin())
		{
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("You need admin privileges to use the /save command.");
			}
			return;
		}
		
		// Use provided name or default
		string saveName = "Manual Save";
		if (data && data.Length() > 0)
		{
			data.Trim();
			saveName = data;
		}
		
		// Show confirmation to admin
		if (panel)
		{
			SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
			if (chatComponent)
				chatComponent.ShowMessage(string.Format("Requesting mission save: %1...", saveName));
		}
		
		// Request save from server via RPC
		m_RplToAuthorityManager.RequestMissionSave(saveName);
	}
	
	//------------------------------------------------------------------------------------------------
	// MAP MARKER SYSTEM
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the array of scripted markers
	 * @return TStringArray - Array of scripted marker data strings
	 */
	TStringArray GetScriptedMarkersArray()
	{
		return m_aScriptedMarkers;
	}

	/**
	 * Adds a scripted marker on the user's map
	 * @param markerEntityName - Name of entity to track (or "Static Marker" for static position)
	 * @param markerOffset - Position offset from entity or static position
	 * @param timeDelay - Update frequency for entity tracking
	 * @param markerText - Text displayed on map
	 * @param markerImage - Image resource path
	 * @param zOrder - Display order/priority
	 * @param markerColor - ARGB color value
	 */
	void AddScriptedMarker(string markerEntityName, string markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.Insert(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset, 
			timeDelay.ToString(), 
			markerText, 
			markerImage, 
			zOrder.ToString(), 
			markerColor.ToString()));
	}

	/**
	 * Removes a specific scripted marker
	 * @param markerEntityName - Name of entity tracked
	 * @param markerOffset - Position offset
	 * @param timeDelay - Update frequency
	 * @param markerText - Text displayed
	 * @param markerImage - Image resource path
	 * @param zOrder - Display order/priority
	 * @param markerColor - ARGB color value
	 */
	void RemoveScriptedMarker(string markerEntityName, string markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.RemoveItemOrdered(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset, 
			timeDelay.ToString(), 
			markerText, 
			markerImage, 
			zOrder.ToString(), 
			markerColor.ToString()));
	}

	/**
	 * Removes all scripted markers
	 */
	void RemoveALLScriptedMarkers()
	{
		m_aScriptedMarkers.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateMapMarkers(array<string> zoneStatus, array<string> zoneObjectNames, FactionKey bluforSide, FactionKey opforSide)
	{
		RemoveALLScriptedMarkers();

		foreach (int i, string zoneName : zoneObjectNames)
		{
			string status = zoneStatus[i];
			string imageTexture;
			int imageColor;

			// Parse zone status
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);

			string zoneLocked = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];

			// Select image based on zone index
			switch (i)
			{
				case 0: {imageTexture = "{21A2A457BD0E42C1}UI\Objectives\A.edds"; break; };
				case 1: {imageTexture = "{7F4A8D140283CCCE}UI\Objectives\B.edds"; break; };
				case 2: {imageTexture = "{8B42CA8C0F5EA4BA}UI\Objectives\C.edds"; break; };
				case 3: {imageTexture = "{C29ADF937D98D0D0}UI\Objectives\D.edds"; break; };
				case 4: {imageTexture = "{3692980B7045B8A4}UI\Objectives\E.edds"; break; };
			}

			// Add lock marker if zone is locked
			if (zoneLocked == "Locked")
				AddScriptedMarker(zoneName, "0 0 0", 0, "", "{91427B7866707601}UI\Objectives\lock.edds", 50, ARGB(255, 142, 142, 142));

			// Set color based on controlling faction
			switch (zoneFactionStored)
			{
				case bluforSide: {imageColor = ARGB(255, 0, 25, 225); break; }; // Blufor
				case opforSide: {imageColor = ARGB(255, 225, 25, 0); break; }; // Opfor
				default: {imageColor = ARGB(255, 225, 225, 225); break; }; // Uncaptured
			}

			// Add zone marker
			AddScriptedMarker(zoneName, "0 0 0", 0, "", imageTexture, 45, imageColor);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void DisplayTitleCard()
	{
		Widget titleCard = GetGame().GetWorkspace().CreateWidgets("{4D2AE199F111C14A}UI/layouts/HUD/Intro/CRF_Intro.layout");
		TextWidget.Cast(titleCard.FindAnyWidget("TitleText")).SetText(CRF_MissionHelper.SanitizeMissionName(GetGame().GetMissionName()));
		AudioSystem.PlaySound("{932C08A5A988F96A}Sounds/Intro/cinematicBoom.wav");
		GetGame().GetCallqueue().CallLater(RemoveWidget, 4000, false, titleCard);
	}
	
	//------------------------------------------------------------------------------------------------
	static void RemoveWidget(Widget widget)
	{
		if (widget)
			widget.RemoveFromHierarchy();
	}
}
