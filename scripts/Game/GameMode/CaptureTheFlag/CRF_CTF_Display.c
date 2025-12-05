//------------------------------------------------------------------------------------
// CRF_CTFDisplay: UI display component for CTF gamemode
// Shows capture progress, flag status, and game information to players
//------------------------------------------------------------------------------------

class CRF_CTFDisplay : SCR_InfoDisplayExtended
{
	protected TextWidget m_wTimer;
	protected TextWidget m_wFlagStatus;
	protected ImageWidget m_wBackground;
	protected CRF_CTFGamemodeManager m_CTFComponent = null;
	
	//------------------------------------------------------------------------------------------------
	// Display update loop
	//------------------------------------------------------------------------------------------------
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		// Get component references if not already cached
		if (!m_CTFComponent || !m_wTimer || !m_wFlagStatus || !m_wBackground) 
		{
			m_CTFComponent = CRF_CTFGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_CTFGamemodeManager));
			m_wTimer = TextWidget.Cast(m_wRoot.FindWidget("Timer"));
			m_wFlagStatus = TextWidget.Cast(m_wRoot.FindWidget("FlagStatus"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("Background"));
			return;
		}
		
		// Check if HUD should be visible
		if (!CRF_PlayerControllerManager.GetInstance().m_bHUDVisible)
		{
			SetVisibility(false);
			return;
		}
		
		SetVisibility(true);
		UpdateDisplay();
	}
	
	//------------------------------------------------------------------------------------------------
	// Update the display with current CTF information
	//------------------------------------------------------------------------------------------------
	protected void UpdateDisplay()
	{
		if (!m_CTFComponent)
			return;
		
		string timerText = "";
		string flagStatusText = "";
		
		// Show capture progress if in progress
		if (m_CTFComponent.IsCaptureInProgress())
		{
			timerText = string.Format("CAPTURING: %1", SCR_FormatHelper.FormatTime(m_CTFComponent.GetCaptureTimeRemaining()));
			
			// Get capturing faction name
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (factionManager)
			{
				Faction faction = factionManager.GetFactionByKey(m_CTFComponent.GetCapturingFaction());
				if (faction)
				{
					flagStatusText = string.Format("%1 is capturing the flag!", faction.GetFactionName());
				}
			}
		}
		else
		{
			// Show flag status
			if (m_CTFComponent.IsFlagCaptured())
			{
				flagStatusText = "Flag has been taken!";
			}
			else
			{
				flagStatusText = "Flag is at base";
			}
			
			timerText = "";
		}
		
		// Update UI elements
		if (m_wTimer)
		{
			m_wTimer.SetText(timerText);
		}
		
		if (m_wFlagStatus)
		{
			m_wFlagStatus.SetText(flagStatusText);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Set overall visibility of the display
	//------------------------------------------------------------------------------------------------
	protected void SetVisibility(bool visible)
	{
        float opacity;
        if (visible)
            opacity = 1.0;
        else
            opacity = 0.0;
		
		if (m_wTimer)
			m_wTimer.SetOpacity(opacity);
		
		if (m_wFlagStatus)
			m_wFlagStatus.SetOpacity(opacity);
		
		if (m_wBackground)
			m_wBackground.SetOpacity(opacity);
	}
}