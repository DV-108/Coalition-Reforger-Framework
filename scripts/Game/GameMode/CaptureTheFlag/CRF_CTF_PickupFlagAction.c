//------------------------------------------------------------------------------------
// CRF_CTFPickupFlagAction: User action for picking up the CTF flag
// Allows players to interact with and pick up the capturable flag
//------------------------------------------------------------------------------------

class CRF_CTFPickupFlagAction : ScriptedUserAction
{
	[Attribute("Capture Flag", desc: "Text shown for the pickup flag action")]
	protected string m_sActionDisplayName;
	
	[Attribute("3.0", desc: "Maximum distance to interact with flag")]
	protected float m_fInteractionDistance;
	
	protected CRF_CTFGamemodeManager m_CTFGamemode;
	protected SCR_FactionManager m_FactionManager;
	
	/**
	 * Initialize the action with necessary manager references
	 * @param pOwnerEntity The entity that owns this action
	 * @param pManagerComponent The action manager component
	 */
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode())
			return;
		
		// Try to initialize managers, but don't fail if they're not ready yet
		// We'll use lazy initialization in CanBeShownScript if needed
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_CTFGamemode = CRF_CTFGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_CTFGamemodeManager));
		
		Print("[CRF_CTFPickupFlagAction] Action initialized for entity: " + pOwnerEntity.GetName());
	}
	
	/**
	 * Get the action priority - higher values appear above lower priority actions
	 * @return Action priority value
	 */
	override float GetActionProgress(IEntity user, void pAction) 
	{
		return 1.0; // Instant action
	}
	
	/**
	 * Get action priority to ensure it shows above default actions
	 * @return Priority value (higher = more priority)
	 */
	override int GetActionPriority()
	{
		return 100; // High priority to show above default actions
	}
	
	/**
	 * Get the display name for this action
	 * @return The action display text
	 */
	override string GetActionDisplayName(IEntity user)
	{
		return m_sActionDisplayName;
	}
	
	/**
	 * Get action name for UI display
	 * @return The action name
	 */
	override string GetActionName()
	{
		return m_sActionDisplayName;
	}
	
	/**
	 * Get action priority to ensure it appears above default actions
	 * @return Priority value (higher = more priority)
	 */
	override int GetActionPriority()
	{
		return 100; // High priority to show above default actions
	}
	
	/**
	 * Get interaction distance for the action
	 * @return Maximum distance for interaction
	 */
	override float GetActionDistance()
	{
		return m_fInteractionDistance;
	}
	
	/**
	 * Perform the pickup flag action
	 * @param pOwnerEntity The flag entity
	 * @param pUserEntity The player attempting to pick up the flag
	 */
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) 
	{
		if (!pOwnerEntity || !pUserEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		// Check if player is valid for flag pickup
		if (!IsPlayerValid(pUserEntity))
			return;
		
		// Get flag component
		CRF_CTFFlagComponent flagComponent = CRF_CTFFlagComponent.Cast(pOwnerEntity.FindComponent(CRF_CTFFlagComponent));
		if (!flagComponent)
		{
			Print("[CRF_CTFPickupFlagAction] ERROR: Flag component not found on entity!", LogLevel.ERROR);
			return;
		}
		
		// Check if flag is already carried
		if (flagComponent.IsCarried())
		{
			Print("[CRF_CTFPickupFlagAction] Flag is already being carried");
			return;
		}
		
		// Attempt to pick up the flag
		if (flagComponent.PickupFlag(pUserEntity))
		{
			Print("[CRF_CTFPickupFlagAction] Flag picked up successfully by: " + pUserEntity.GetName());
		}
		else
		{
			Print("[CRF_CTFPickupFlagAction] Failed to pick up flag");
		}
	}
	
	/**
	 * Check if the action can be shown to the user
	 * @param user The user entity
	 * @return True if action should be visible
	 */
	override bool CanBeShownScript(IEntity user)
	{
		if (!user)
			return false;
		
		// Get managers if not already initialized (lazy initialization)
		if (!m_FactionManager)
			m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!m_CTFGamemode)
			m_CTFGamemode = CRF_CTFGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_CTFGamemodeManager));
		
		// Early exit if managers not available
		if (!m_FactionManager || !m_CTFGamemode)
			return false;
		
		// Get the owner entity (flag)
		IEntity ownerEntity = GetOwner();
		if (!ownerEntity)
			return false;
		
		// Check if flag is already carried
		CRF_CTFFlagComponent flagComponent = CRF_CTFFlagComponent.Cast(ownerEntity.FindComponent(CRF_CTFFlagComponent));
		if (!flagComponent)
			return false;
		
		if (flagComponent.IsCarried())
			return false;
		
		// Check if player is valid for flag pickup
		return IsPlayerValid(user);
	}
	
	/**
	 * Check if the action can be performed
	 * @param user The player attempting the action
	 * @return True if action can be performed
	 */
	override bool CanBePerformedScript(IEntity user)
	{
		// Use the same logic as CanBeShownScript for consistency
		return CanBeShownScript(user);
	}
	
	/**
	 * Get the display name for the action
	 * @return The localized action name
	 */
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
	
	/**
	 * Check if player is valid for flag pickup (alive, correct faction, etc.)
	 * @param player The player entity to check
	 * @return True if player can pick up the flag
	 */
	protected bool IsPlayerValid(IEntity player)
	{
		if (!player)
			return false;
		
		// Ensure managers are available
		if (!m_FactionManager || !m_CTFGamemode)
			return false;
		
		// Check if player is alive
		ChimeraCharacter character = ChimeraCharacter.Cast(player);
		if (!character)
			return false;
		
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return false;
		
		// Check if player is alive and not unconscious
		if (controller.IsDead() || controller.IsUnconscious())
			return false;
		
		// Check if player belongs to a valid faction
		SCR_FactionAffiliationComponent factionComp = SCR_FactionAffiliationComponent.Cast(player.FindComponent(SCR_FactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction faction = factionComp.GetAffiliatedFaction();
		if (!faction)
			return false;
		
		// Check if CTF gamemode allows this faction to participate
		FactionKey factionKey = faction.GetFactionKey();
		switch (factionKey)
		{
			case "BLUFOR":
				return m_CTFGamemode.IsBluforEnabled();
			case "OPFOR":
				return m_CTFGamemode.IsOpforEnabled();
			case "INDFOR":
				return m_CTFGamemode.IsIndforEnabled();
		}
		
		return false;
	}
	
	/**
	 * Override to ensure proper action networking
	 */
	override bool CanBroadcastScript()
	{
		return true; // Action should be networked to other players
	}
}