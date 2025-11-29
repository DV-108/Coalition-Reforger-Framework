class CRF_AirdropUI: ChimeraMenuBase
{
	Widget m_wRoot;
	
	string m_sPlayerFactionKey;
	
	TextWidget m_FlightNumberText;
	TextWidget m_DistanceToGreenLightText;
	TextWidget m_FlightpathDistanceText;
	TextWidget m_AltitudeText;
	TextWidget m_PlayersText;
	TextWidget m_FlightPathText;
	
	SCR_ButtonComponent m_CreateFlightButton;
	SCR_ButtonComponent m_CreateFlightpathButton;
	SCR_ButtonComponent m_LowAltitudeButton;
	SCR_ButtonComponent m_MediumAltitudeButton;
	SCR_ButtonComponent m_HighAltitudeButton;
	
	VerticalLayoutWidget m_FlightList;
	VerticalLayoutWidget m_UnassignedList;
	VerticalLayoutWidget m_AssignedList;
	VerticalLayoutWidget m_FlightPathList;
	
	Widget m_FlightPath;
	
	WorkspaceWidget m_Workspace ;
	
	ref array<int> m_aAssignedSquads = {};
	ref array<ref CRF_Flightpath> m_aFlightPaths = {};
	bool m_bUpdateMenu = false;
	
	override void OnMenuOpen()
	{
		m_wRoot = GetRootWidget();
		m_CreateFlightButton = SCR_ButtonComponent.Cast(m_wRoot.FindAnyWidget("CreateFlightButton").FindHandler(SCR_ButtonComponent));
		m_CreateFlightpathButton = SCR_ButtonComponent.Cast(m_wRoot.FindAnyWidget("CreateFlightpathButton").FindHandler(SCR_ButtonComponent));
		m_LowAltitudeButton = SCR_ButtonComponent.Cast(m_wRoot.FindAnyWidget("LowAltitudeButton").FindHandler(SCR_ButtonComponent));
		m_MediumAltitudeButton = SCR_ButtonComponent.Cast(m_wRoot.FindAnyWidget("MediumAltitudeButtton").FindHandler(SCR_ButtonComponent));
		m_HighAltitudeButton = SCR_ButtonComponent.Cast(m_wRoot.FindAnyWidget("HighAltitudeButton").FindHandler(SCR_ButtonComponent));
		
		m_FlightList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("FlightList"));
		m_UnassignedList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("UnassignedList"));
		m_AssignedList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("AssignedList"));
		m_FlightPathList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("FlightPathList"));
		
		m_FlightNumberText = TextWidget.Cast(m_wRoot.FindAnyWidget("FlightNumber"));
		m_DistanceToGreenLightText = TextWidget.Cast(m_wRoot.FindAnyWidget("GreenLight"));
		m_FlightpathDistanceText = TextWidget.Cast(m_wRoot.FindAnyWidget("Distance"));
		m_AltitudeText = TextWidget.Cast(m_wRoot.FindAnyWidget("Altitude"));
		m_PlayersText = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersCount"));
		m_FlightPathText = TextWidget.Cast(m_wRoot.FindAnyWidget("SelectedFlightPath"));
		
		m_FlightPath = m_wRoot.FindAnyWidget("FlightPath");
		
		m_Workspace = GetGame().GetWorkspace();
		
		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		if (!playerFaction)
			Close();
		
		m_sPlayerFactionKey = playerFaction.GetFactionKey();
		 
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		if (m_bUpdateMenu)
		{
			m_bUpdateMenu = false;
			UpdateMenuLists();
		}
	}
	
	void UpdateMenuLists()
	{
		while (m_AssignedList.GetChildren())
			delete m_AssignedList.GetChildren();
		
		while (m_UnassignedList.GetChildren())
			delete m_UnassignedList.GetChildren();
		
		while (m_FlightPathList.GetChildren())
			delete m_FlightPathList.GetChildren();
		
		DrawSquads();
		DrawFlightPaths();
	}
	
	void DrawFlightPaths()
	{
		foreach (int i, CRF_Flightpath flightPath: m_aFlightPaths)
		{
			Widget flightPathWidget = m_Workspace.CreateWidgets("{CBDC134471A8B7D8}UI/layouts/Map/Airdrops/AirdropFlighpathItem.layout", m_FlightPathList);
			TextWidget.Cast(flightPathWidget.FindAnyWidget("FlightpathText")).SetText("Flightpath #" + (i + 1).ToString());
		}
	}
	
	void DrawSquads()
	{
		SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsMan)
			return;
		
		array<SCR_AIGroup> groups = {};
		groupsMan.GetAllPlayableGroups(groups);
		
		foreach (SCR_AIGroup group: groups)
		{
			if (!group.GetFaction())
				continue;
			
			if (group.GetFaction().GetFactionKey() != m_sPlayerFactionKey)
				continue;
			
			if (m_aAssignedSquads.Contains(group.GetGroupID()))
				DrawGroupObject(group, true);
			else
				DrawGroupObject(group, false);
		}
	}
	
	void DrawGroupObject(SCR_AIGroup group, bool isAssigned)
	{
		Widget list;
		
		if (isAssigned)
			list = m_AssignedList;
		else
			list = m_UnassignedList;
		
		SCR_EditableGroupUIInfo groupUIInfo = SCR_EditableGroupUIInfo.Cast(group.FindComponent(SCR_EditableGroupUIInfo));
		if (!groupUIInfo)
			return;
		
		Widget newObject = m_Workspace.CreateWidgets("{127B18DC471C3790}UI/layouts/Map/Airdrops/AirdropSquadItem.layout", list);
		SCR_MilitarySymbolUIComponent militarySymbol = SCR_MilitarySymbolUIComponent.Cast(newObject.FindAnyWidget("Icon").FindHandler(SCR_MilitarySymbolUIComponent));
		militarySymbol.Update(groupUIInfo.GetMilitarySymbol());
		
		string company, platoon, squad, character, format;
		group.GetCallsigns(company, platoon, squad, character, format);
		
		TextWidget.Cast(newObject.FindAnyWidget("SquadName")).SetText(squad);
		
		TextWidget.Cast(newObject.FindAnyWidget("SquadSlots")).SetText(group.GetPlayerCount(true).ToString());
	}
}