class CRF_ClientInitializationManagerClass : ScriptComponentClass {}
class CRF_ClientInitializationManager : ScriptComponent
{
	protected static CRF_ClientInitializationManager m_sInstance;
	void CRF_ClientInitializationManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_ClientInitializationManager GetInstance()
	{
		return m_sInstance;
	}
	
	/**
	 * Opens appropriate menu based on current gamemode state
	 */
	void OpenCurrentStateMenu()
	{	
		// Initialize references first
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		// Check if we should skip AAR
		if (m_Gamemode && m_Gamemode.m_GamemodeState == CRF_EGamemodeState.AAR && !m_Gamemode.m_bUseAAR)
			return;
		
		// Close any existing menus
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();
		GetGame().GetMenuManager().CloseAllMenus();
		
		// Open appropriate menu based on gamemode state
		switch (m_Gamemode.m_GamemodeState)
		{
			case CRF_EGamemodeState.BRIEFING: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);
				break;
			}
			case CRF_EGamemodeState.SLOTTING:
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
				break;
			}
			case CRF_EGamemodeState.GAME: 
			{
				m_RplToAuthorityManager.RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
				break;
			}
			case CRF_EGamemodeState.AAR: 
			{
				if (CRF_Gamemode.GetInstance().m_bUseAAR)
					GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_AARMenu);
				break;
			}
		}
	}
	
	/**
	 * Initializes the player client
	 * Cleans up previous camera, closes menus, and sets up player-specific settings
	 * @param playerCharacter - The spectator entity the server created and set to this player
	 */
	void InitilizePlayerClient(RplId playerCharID)
	{
		// Get player character
		IEntity playerCharacter = m_SlottingManager.GetCharacterFromRplId(playerCharID);
		
		// if we cant get the player character or it's null, wait another full initilization time before attempting again
		if (!playerCharacter || !SCR_ChimeraCharacter.Cast(playerCharacter))
		{
			// Schedule another verification attempt
			GetGame().GetCallqueue().CallLater(InitilizePlayerClient, CRF_GamemodeManager.PLAYER_INITILIZATION_TIME, false, playerCharID);
			return;
		};
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
		
		// Close all menus
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			GetGame().GetMenuManager().CloseAllMenus();
			if (!CVON_VONGameModeComponent.GetInstance())
				SetupRadioFrequency();
		}; 
		
		if (playerCharacter.GetPrefabData().GetPrefabName() == CRF_GamemodeManager.GetSpectatorResource())
			InitilizeLocalSpectator(playerCharacter);
		else
			InitilizeLocalCharacter();
	}
	
	/**
	 * Initilizes players if they have a valid spectator entity
	 * @param playerCharacter - The spectator entity the server created and set to this player
	 */
	void InitilizeLocalSpectator(IEntity playerCharacter)
	{
		m_CameraManager.InitilizeSpecCamera();
		
		// Register for VON (voice chat)
		m_RplToAuthorityManager.CheckVONRegister(SCR_PlayerController.GetLocalPlayerId());
		
		// Open spectator menu if in game state
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SpectatorMenu);
		
		// Turn on killfeed for specs
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeDeadLocal();
	}
	
	/**
	 * Initilizes players if they have a valid slotted character
	 */
	void InitilizeLocalCharacter()
	{
		// Clean up previous camera if exists
		if (m_CameraManager.m_eCamera)
			delete m_CameraManager.m_eCamera;
		
		// Originally added for data collector
		m_Gamemode.GetOnPlayerSpawned().Invoke(SCR_PlayerController.GetLocalPlayerId(), SCR_PlayerController.GetLocalMainEntity());
		
		// Reset Stored Pos
		GetGame().GetCallqueue().CallLater(m_CameraManager.UpdateStoredCameraPos, 200, false, vector.Zero, vector.Zero, vector.Zero, vector.Zero);
		
		// Reset kill feed type to default
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeNoneLocal();
	}
	
	/**
	 * Sets up radio frequencies based on player group
	 * Configures both group and platoon frequencies
	 */
	void SetupRadioFrequency()
	{
		// Get player's entity
		IEntity entity = SCR_PlayerController.GetLocalMainEntity();
		if (!entity || CRF_GamemodeManager.IsSpectator(entity))
			return;

		// Find radio in inventory
		array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent)).GetItems(items);
		IEntity radioEntity;
		foreach (IEntity item : items)
		{
			if (item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return;

		// Get radio components
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		BaseTransceiver grpTsv = radio.GetTransceiver(0);

		// Get player's group
		SCR_GroupsManagerComponent m_GroupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!m_GroupManager)
			return;

		SCR_AIGroup group = m_GroupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		PlayerController pc = GetGame().GetPlayerController();

		// Set frequency based on group
		if (pc && group)
		{
			grpTsv.SetFrequency(group.GetRadioFrequency());
		}

		// Set up Voice over Network component
		SCR_VONController vc = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		SCR_VoNComponent von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));

		von.SetTransmitRadio(grpTsv);

		// Set up platoon radio if available
		BaseTransceiver pltTsv = radio.GetTransceiver(1);
		if (pltTsv)
			von.SetTransmitRadio(pltTsv);

		vc.PublicResetVON();
		vc.SetVONComponent(von);
	}
	
	//------------------------------------------------------------------------------------------------
	void DisableAI(IEntity owner)
	{
		AIControlComponent aiComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (!agent)
			return;
		
		agent.DeactivateAI();
		
		// Double-check deactivation next frame
		GetGame().GetCallqueue().Call(DisableAIWrap, owner, aiComponent);
	}

	//------------------------------------------------------------------------------------------------
	void DisableAIWrap(IEntity owner, AIControlComponent aiComponent)
	{
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (agent)
			agent.DeactivateAI();
	}
}