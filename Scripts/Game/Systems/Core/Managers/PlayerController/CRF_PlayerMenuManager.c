class CRF_PlayerMenuManagerClass : ScriptComponentClass {}
class CRF_PlayerMenuManager : ScriptComponent
{
	protected static CRF_PlayerMenuManager m_sInstance;
	void CRF_PlayerMenuManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_PlayerMenuManager GetInstance()
	{
		return m_sInstance;
	}	
	
	/**
	 * Opens appropriate menu based on current gamemode state
	 */
	void OpenCurrentStateMenu()
	{	
		// Close any existing menus
		GetGame().GetMenuManager().CloseAllMenus();
		
		// Open appropriate menu based on gamemode state
		switch (CRF_Gamemode.GetInstance().m_GamemodeState)
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
		}
	}
};