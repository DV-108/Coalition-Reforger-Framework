/**
 * Custom Map Menu UI class for the Coalition Reforger Framework
 * Extends the default map menu to provide mission description functionality
 * This class initializes the mission description list when clients open their map
 */
modded class SCR_MapMenuUI
{
	//----------------------------------------
	// UI Components
	//----------------------------------------
	protected SCR_ListBoxComponent m_cMissionDescriptionListBoxComponent; // Component for mission descriptions
	protected ButtonWidget m_wBackButton;                                 // Back button for description navigation
	protected CRF_Gamemode m_Gamemode;                                   // Game mode instance
	protected ref array<ref CRF_MissionDescriptor> m_aActiveDescriptors = {}; // Active mission descriptors
	protected bool m_bMissionDescriptionsInitialized = false;             // Flag to track if descriptions have been initialized
	
	protected TextWidget m_FlightHint;
	protected Widget m_FlightPath;
	protected vector m_vCachedDir;
	protected float  m_fCachedLengthSq;
	protected bool m_bGreenLightLocked = false;
	protected float m_fGreenLightT = 0.0;   // 0..1 along the path
	vector m_vStartPoint;
	vector m_vEndPoint;
	vector m_vGreenPoint;

	//----------------------------------------
	// Menu Lifecycle Methods
	//----------------------------------------

	/**
	 * Called when the map menu is opened
	 * Initializes mission description functionality only on first open
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_FlightHint = TextWidget.Cast(GetRootWidget().FindAnyWidget("FlightHintText"));
		// Don't initialize on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			return;
		}

		// Only initialize once on first map open
		if (m_bMissionDescriptionsInitialized) {
			return;
		}

		// Initialize gamemode reference
		m_Gamemode = CRF_Gamemode.GetInstance();
		if (!m_Gamemode) {
			return;
		}

		// Initialize mission description components
		InitializeMissionDescriptions();
		
		// Mark as initialized to prevent re-initialization
		m_bMissionDescriptionsInitialized = true;
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (m_MapEntity)
			if (m_MapEntity.m_bCreatingFlight)
			{
				m_FlightHint.GetParent().SetVisible(true);
				
				FlightCreationUpdate();
			}
	}
	
	void FlightCreationUpdate()
	{
		switch (m_MapEntity.m_iPhase)
		{
			case 0:
			m_FlightHint.SetText("Select Start Point");
			break;
			
			case 1:
			m_FlightHint.SetText("Select End Point");
			break;
			
			case 2:
			m_FlightHint.SetText("Select Greenlight Position");
			if (m_FlightPath)
				delete m_FlightPath;
			
			m_FlightPath = GetGame().GetWorkspace().CreateWidgets("{8C3DFB5FE48D9901}UI/layouts/Map/Airdrops/AirdropMapFlightPath.layout", GetRootWidget());
			float sX, sZ;
			m_vStartPoint = m_MapEntity.m_vFlightStart;
			m_vEndPoint = m_MapEntity.m_vFlightEnd;
			m_MapEntity.WorldToScreen(m_MapEntity.m_vFlightStart[0], m_MapEntity.m_vFlightStart[2], sX, sZ, true);
			float eX, eZ;
			m_MapEntity.WorldToScreen(m_MapEntity.m_vFlightEnd[0], m_MapEntity.m_vFlightEnd[2], eX, eZ, true);
			
			PlaceWidgetBetweenPoints(m_FlightPath.FindAnyWidget("RedLight"), Vector(sX, 0, sZ), Vector(eX, 0, eZ));
			CacheFlightBarTransform();
			vector cursorWorld;
			float cX, cY;
			m_MapEntity.GetMapCursorWorldPosition(cX, cY);
			cursorWorld = Vector(cX, 0, cY);
			m_fGreenLightT = GetGreenLightFromCursor(cursorWorld);
			UpdateGreenLightPosition(m_fGreenLightT);
			break;	
			case 3:
			
			break;
		}
	}
	
	void PlaceWidgetBetweenPoints(Widget w, vector p0, vector p1)
	{
		float x0 = GetGame().GetWorkspace().DPIUnscale(p0[0]);
		float y0 = GetGame().GetWorkspace().DPIUnscale(p0[2]);
	
		float x1 = GetGame().GetWorkspace().DPIUnscale(p1[0]);
		float y1 = GetGame().GetWorkspace().DPIUnscale(p1[2]);
	
		float midX = (x0 + x1) * 0.5;
		float midY = (y0 + y1) * 0.5;
	
		float dx = x1 - x0;
		float dy = y1 - y0;
		float angleDeg = Math.Atan2(dy, dx) * Math.RAD2DEG;
	
		float distance = Math.Sqrt(dx * dx + dy * dy);
	
		float h = FrameSlot.GetSizeY(w);
	
		FrameSlot.SetSize(w, distance, h);
	
		float posX = midX - (FrameSlot.GetSizeX(w) * 0.5);
		float posY = midY - (FrameSlot.GetSizeY(w) * 0.5);
		FrameSlot.SetPos(w, posX, posY);
	
		ImageWidget.Cast(w).SetRotation(angleDeg);
	}
	
	void UpdateGreenLightPosition(float t)
{
	// --- World → screen endpoints
	float sX, sZ;
	float eX, eZ;

	m_MapEntity.WorldToScreen(
		m_MapEntity.m_vFlightStart[0],
		m_MapEntity.m_vFlightStart[2],
		sX, sZ, true
	);

	m_MapEntity.WorldToScreen(
		m_MapEntity.m_vFlightEnd[0],
		m_MapEntity.m_vFlightEnd[2],
		eX, eZ, true
	);

	// --- DPI unscale
	sX = GetGame().GetWorkspace().DPIUnscale(sX);
	sZ = GetGame().GetWorkspace().DPIUnscale(sZ);
	eX = GetGame().GetWorkspace().DPIUnscale(eX);
	eZ = GetGame().GetWorkspace().DPIUnscale(eZ);

	// --- Flight vector
	float dx = eX - sX;
	float dy = eZ - sZ;
	float distance = Math.Sqrt(dx * dx + dy * dy);
	// --- Clamp t so LEFT EDGE never overshoots the ends
	t = Math.Clamp(t, 0.0, 1.0);

	// --- LEFT-CENTER anchor (cursor controls LEFT EDGE)
	float leftX = sX + dx * t;
	float leftY = sZ + dy * t;

	Widget green = m_FlightPath.FindAnyWidget("GreenLight");

	float h = FrameSlot.GetSizeY(green);

	// ✅ Size grows ONLY to the right
	FrameSlot.SetSize(green, distance, h);

	// --- TRUE center-left anchoring (center shifted forward by half width)
	float halfW = FrameSlot.GetSizeX(green) * 0.5;
	float angleDeg = Math.Atan2(dy, dx) * Math.RAD2DEG;
	
	float posX = leftX + Math.Cos(angleDeg * Math.DEG2RAD) * halfW;
	float posY = leftY + Math.Sin(angleDeg * Math.DEG2RAD) * halfW;
	
	FrameSlot.SetPos(green,
		posX - FrameSlot.GetSizeX(green) * 0.5,
		posY - FrameSlot.GetSizeY(green) * 0.5
	);

	ImageWidget.Cast(green).SetRotation(angleDeg);
}


	void CacheFlightBarTransform()
	{
		m_vCachedDir = m_MapEntity.m_vFlightEnd - m_MapEntity.m_vFlightStart;
	
		m_fCachedLengthSq =
			m_vCachedDir[0] * m_vCachedDir[0] +
			m_vCachedDir[2] * m_vCachedDir[2];
	}
	
	float GetGreenLightFromCursor(vector cursorWorldPos)
	{
		vector toCursor = cursorWorldPos - m_MapEntity.m_vFlightStart;
	
		// Safety in case start == end
		if (m_fCachedLengthSq <= 0.001)
			return 0.0;
	
		float dot =
			toCursor[0] * m_vCachedDir[0] +
			toCursor[2] * m_vCachedDir[2];
	
		float t = dot / m_fCachedLengthSq;
	
		return Math.Clamp(t, 0.0, 1.0);
	}

	/**
	 * Called when the map menu is closed
	 * Cleanup mission description components
	 */
	override void OnMenuClose()
	{
		if (m_FlightPath)
			delete m_FlightPath;
		
		super.OnMenuClose();

		// Clear mission description data
		if (m_cMissionDescriptionListBoxComponent) {
			m_cMissionDescriptionListBoxComponent.Clear();
			m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();
		}
		
		if (m_aActiveDescriptors) {
			m_aActiveDescriptors.Clear();
		}
	}

	//----------------------------------------
	// Mission Description Methods
	//----------------------------------------

	/**
	 * Initialize mission description components and populate the list
	 */
	protected void InitializeMissionDescriptions()
	{
		// Find the MissionDescription widget in the map menu layout
		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (!missionDescriptionWidget) {
			return;
		}

		// Find the DescriptionList within the MissionDescription widget
		OverlayWidget descriptionListWidget = OverlayWidget.Cast(missionDescriptionWidget.FindAnyWidget("DescriptionList"));
		if (!descriptionListWidget) {
			return;
		}

		// Get the list box component
		m_cMissionDescriptionListBoxComponent = SCR_ListBoxComponent.Cast(descriptionListWidget.FindHandler(SCR_ListBoxComponent));
		if (!m_cMissionDescriptionListBoxComponent) {
			return;
		}

		// Find the back button
		m_wBackButton = ButtonWidget.Cast(missionDescriptionWidget.FindAnyWidget("BackButton"));
		if (m_wBackButton) {
			m_wBackButton.SetOpacity(0);
		}

		// Initialize the mission description list
		DescriptionInit();
	}

	/**
	 * Initialize the mission description section
	 * Populates the list with mission descriptors relevant to the player's faction
	 */
	void DescriptionInit()
	{
		if (!m_cMissionDescriptionListBoxComponent || !m_Gamemode) {
			return;
		}

		// Find scroll layout and disable it initially
		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (!missionDescriptionWidget) {
			return;
		}

		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(missionDescriptionWidget.FindAnyWidget("ScrollLayout"));
		if (scrollLayout) {
			scrollLayout.SetEnabled(false);
		}

		// Reset back button
		if (m_wBackButton) {
			m_wBackButton.SetOpacity(0);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton) {
				backButton.m_OnClicked.Clear();
			}
		}

		// Clear description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(missionDescriptionWidget.FindAnyWidget("DescriptionInfo"));
		if (missionDescriptionText) {
			missionDescriptionText.SetText("");
		}

		// Clear list components
		m_cMissionDescriptionListBoxComponent.Clear();
		m_aActiveDescriptors.Clear();

		// Get player faction
		SCR_PlayerFactionAffiliationComponent factionComponent = SCR_PlayerFactionAffiliationComponent.Cast(
			GetGame().GetPlayerController().FindComponent(SCR_PlayerFactionAffiliationComponent)
		);

		if (!factionComponent) {
			return;
		}

		string playerFaction = factionComponent.GetAffiliatedFactionKey();

		// Add relevant descriptions to list
		foreach (ref CRF_MissionDescriptor description : m_Gamemode.m_aMissionDescriptors)
		{
			// Add description visible to all factions
			if (description.m_bShowForAnyFaction)
			{
				m_cMissionDescriptionListBoxComponent.AddItem(
					description.m_sTitle, 
					null, 
					"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
				);
				m_aActiveDescriptors.Insert(description);
				continue;
			}
			
			// Add description specific to player's faction
			foreach (string factionKey : description.m_aFactionKeys)
			{
				if (playerFaction == factionKey)
				{
					m_cMissionDescriptionListBoxComponent.AddItem(
						description.m_sTitle, 
						null, 
						"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
					);
					m_aActiveDescriptors.Insert(description);
					break;
				}
			}
		}

		// Register selection handler
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Insert(DescriptionSelected);
	}

	/**
	 * Handles selection of a description item
	 * Shows the selected description text and enables navigation
	 */
	void DescriptionSelected()
	{
		if (!m_cMissionDescriptionListBoxComponent || !m_aActiveDescriptors) {
			return;
		}

		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (!missionDescriptionWidget) {
			return;
		}

		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(missionDescriptionWidget.FindAnyWidget("ScrollLayout"));
		if (scrollLayout) {
			scrollLayout.SetEnabled(true);
		}

		// Get selected description
		int index = m_cMissionDescriptionListBoxComponent.GetSelectedItem();
		if (index < 0 || index >= m_aActiveDescriptors.Count()) {
			return;
		}
			
		string description = m_aActiveDescriptors.Get(index).m_sTextData;

		// Show back button
		if (m_wBackButton) {
			m_wBackButton.SetOpacity(1);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton) {
				backButton.m_OnClicked.Insert(DescriptionInit);
			}
		}

		// Clear description list
		m_cMissionDescriptionListBoxComponent.Clear();
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();

		// Set description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(missionDescriptionWidget.FindAnyWidget("DescriptionInfo"));
		if (missionDescriptionText) {
			missionDescriptionText.SetText(description);
		}
	}
}
