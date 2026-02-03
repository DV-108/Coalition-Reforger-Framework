class CRF_PlayerInitializationManagerClass : ScriptComponentClass {}
class CRF_PlayerInitializationManager : ScriptComponent
{
	protected CRF_Gamemode m_Gamemode;
	protected CRF_RplToAuthorityManager m_RplToAuthorityManager;
	protected CRF_Gamemode m_Gamemode;
	
	protected static CRF_PlayerInitializationManager m_sInstance;
	void CRF_PlayerInitializationManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_PlayerInitializationManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
	
		// Only run on in-game post init
		if (!GetGame().InPlayMode())
			return;
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
	}	
	
	/**
	 * Initializes the player client
	 * Cleans up previous camera, closes menus, and sets up player-specific settings
	 * @param playerCharacter - The spectator entity the server created and set to this player
	 */
	void InitilizePlayerClient()
	{		
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
		m_CameraManager.RemoveCamera();
		
		// Originally added for data collector
		m_Gamemode.GetOnPlayerSpawned().Invoke(SCR_PlayerController.GetLocalPlayerId(), SCR_PlayerController.GetLocalMainEntity());
		
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