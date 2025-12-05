//------------------------------------------------------------------------------------
// CRF_CTFDropZoneComponent: Component for team drop-off zones in CTF gamemode
// Handles detection of flag carriers entering/leaving zones and capture timing
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "CTF Components", description: "CTF Drop Zone component with capture detection")]
class CRF_CTFDropZoneComponentClass: ScriptComponentClass
{
	
}

class CRF_CTFDropZoneComponent: ScriptComponent
{
	//===================================================================================
	// ATTRIBUTES
	//===================================================================================
	
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")}, desc: "Which faction this drop zone belongs to (can be set dynamically by gamemode)")]
	FactionKey m_FactionKey;
	
	[Attribute("25", desc: "Radius of the drop zone in meters")]
	float m_fZoneRadius;
	
	//===================================================================================
	// PROPERTIES
	//===================================================================================
	
	protected CRF_CTFGamemodeManager m_CTFGamemode;
	protected SCR_FactionManager m_FactionManager;
	protected IEntity m_CurrentFlagCarrier;
	protected bool m_bCarrierInZone = false;
	
	// For entity queries
	protected ref array<IEntity> m_EntitiesInRange;
	protected IEntity m_FoundFlagCarrier;
	
	//===================================================================================
	// INITIALIZATION
	//===================================================================================
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!GetGame().InPlayMode())
			return;
		
		// Get manager references
		m_CTFGamemode = CRF_CTFGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_CTFGamemodeManager));
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		// Start monitoring for flag carriers
		GetGame().GetCallqueue().CallLater(CheckForCarriers, 500, true);
		
		Print("[CRF_CTFDropZoneComponent] Drop zone initialized for faction: " + m_FactionKey);
	}
	
	//===================================================================================
	// ZONE MONITORING
	//===================================================================================
	
	/**
	 * Callback function for entity sphere query
	 * @param entity The entity found in the sphere
	 * @return True to continue query
	 */
	protected bool FilterEntitiesCallback(IEntity entity)
	{
		if (!entity)
			return true;
		
		// Check if this is a valid flag carrier for our faction
		if (IsValidFlagCarrier(entity))
		{
			m_FoundFlagCarrier = entity;
			return false; // Stop searching once we find one
		}
		
		return true; // Continue searching
	}
	
	/**
	 * Periodically check for flag carriers in the zone
	 */
	protected void CheckForCarriers()
	{
		if (!m_CTFGamemode || !m_FactionManager)
			return;
		
        //Print("[CRF_CTFDropZoneComponent] Checking for flag carriers in zone...");

		// Reset search variables
		m_FoundFlagCarrier = null;
		vector zonePos = GetOwner().GetOrigin();
		
		// Find all entities within zone radius
		GetGame().GetWorld().QueryEntitiesBySphere(zonePos, m_fZoneRadius, FilterEntitiesCallback, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);
		
        //Print("[CRF_CTFDropZoneComponent] m_FoundFlagCarrier: " + m_FoundFlagCarrier);
		// Handle carrier entering/leaving zone
		if (m_FoundFlagCarrier && !m_bCarrierInZone)
		{
			OnCarrierEntered(m_FoundFlagCarrier);
		}
		else if (!m_FoundFlagCarrier && m_bCarrierInZone)
		{
			OnCarrierLeft();
		}
		
		m_CurrentFlagCarrier = m_FoundFlagCarrier;
		m_bCarrierInZone = (m_FoundFlagCarrier != null);
	}
	
	/**
	 * Check if an entity is a valid flag carrier for this drop zone's faction
	 * @param entity The entity to check
	 * @return True if entity is carrying flag and belongs to correct faction
	 */
	protected bool IsValidFlagCarrier(IEntity entity)
	{
		if (!entity)
			return false;
		
		// Check if entity is a character
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return false;
		
		// Check if player is alive
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));
		if (!controller || controller.IsDead() || controller.IsUnconscious())
			return false;
		
		// Check if player belongs to the correct faction
		SCR_FactionAffiliationComponent factionComp = SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction faction = factionComp.GetAffiliatedFaction();
		if (!faction || faction.GetFactionKey() != m_FactionKey)
			return false;
		
		// Check if player is carrying the flag
		// We need to check with the CTF gamemode if this player is the flag carrier
		if (!m_CTFGamemode)
			return false;
		
		return m_CTFGamemode.IsPlayerCarryingFlag(entity);
	}
	
	//===================================================================================
	// ZONE EVENTS
	//===================================================================================
	
	/**
	 * Called when a valid flag carrier enters the zone
	 * @param carrier The flag carrier entity
	 */
	protected void OnCarrierEntered(IEntity carrier)
	{
		if (!m_CTFGamemode || !carrier)
			return;
		
		Print("[CRF_CTFDropZoneComponent] Flag carrier entered " + m_FactionKey + " drop zone");
		
		// Notify gamemode to start capture process
		m_CTFGamemode.StartCapture(carrier, GetOwner());
	}
	
	/**
	 * Called when the flag carrier leaves the zone
	 */
	protected void OnCarrierLeft()
	{
		if (!m_CTFGamemode)
			return;
		
		Print("[CRF_CTFDropZoneComponent] Flag carrier left " + m_FactionKey + " drop zone");
		
		// Notify gamemode to stop capture process
		m_CTFGamemode.StopCapture();
	}
	
	//===================================================================================
	// GETTERS
	//===================================================================================
	
	/**
	 * Get the faction this drop zone belongs to
	 * @return The faction key
	 */
	FactionKey GetFactionKey()
	{
		return m_FactionKey;
	}
	
	/**
	 * Set the faction this drop zone belongs to (for dynamic configuration)
	 * @param factionKey The faction key to assign
	 */
	void SetFactionKey(FactionKey factionKey)
	{
		m_FactionKey = factionKey;
		Print("[CRF_CTFDropZoneComponent] Faction set to: " + m_FactionKey);
	}
	
	/**
	 * Check if a flag carrier is currently in the zone
	 * @return True if carrier is in zone
	 */
	bool HasCarrierInZone()
	{
		return m_bCarrierInZone;
	}
	
	/**
	 * Get the current flag carrier in the zone
	 * @return The carrier entity or null
	 */
	IEntity GetCarrierInZone()
	{
		return m_CurrentFlagCarrier;
	}
	
	//===================================================================================
	// CLEANUP
	//===================================================================================
	
	override void OnDelete(IEntity owner)
	{
		// Stop monitoring timer
		GetGame().GetCallqueue().Remove(CheckForCarriers);
		
		super.OnDelete(owner);
	}
}