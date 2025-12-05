//------------------------------------------------------------------------------------
// CRF_CTFFlagComponent: Component for the capturable flag in CTF gamemode
// Handles flag pickup, drop, and carrier tracking functionality
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "CTF Components", description: "CTF Flag component with pickup/drop mechanics")]
class CRF_CTFFlagComponentClass: ScriptComponentClass
{
	
}

class CRF_CTFFlagComponent: ScriptComponent
{
	//===================================================================================
	// PROPERTIES
	//===================================================================================
	
	protected CRF_CTFGamemodeManager m_CTFGamemode;
	protected IEntity m_CurrentCarrier;
	protected bool m_bIsCarried = false;
	
	//===================================================================================
	// INITIALIZATION
	//===================================================================================
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!GetGame().InPlayMode())
			return;
		
		// Get reference to CTF gamemode
		m_CTFGamemode = CRF_CTFGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_CTFGamemodeManager));
		
		// Register for player death events if we're on the server
		if (RplSession.Mode() == RplMode.Dedicated || RplSession.Mode() == RplMode.Listen)
		{
			// Start monitoring for carrier death/disconnect
			GetGame().GetCallqueue().CallLater(MonitorCarrierStatus, 1000, true); // Check every second
		}
	}
	
	//===================================================================================
	// FLAG MECHANICS
	//===================================================================================
	
	/**
	 * Called when a player attempts to pick up the flag
	 * @param player The player entity trying to pick up the flag
	 * @return True if pickup was successful
	 */
	bool PickupFlag(IEntity player)
	{
		if (!player || m_bIsCarried)
			return false;
		
		// Check if gamemode allows flag pickup
		if (!m_CTFGamemode)
			return false;
		
		m_CurrentCarrier = player;
		m_bIsCarried = true;
		
		// Notify gamemode of flag pickup
		m_CTFGamemode.FlagPickedUp(player);
		
		// Attach flag to player (visual representation)
		AttachToPlayer(player);
		
		// Send hint notification to all clients
		SendFlagPickupNotification(player);
		
		Print("[CRF_CTFFlagComponent] Flag picked up by: " + player.GetName());
		return true;
	}
	
	/**
	 * Called when the flag is dropped
	 * @param position Position where flag should be dropped
	 */
	void DropFlag(vector position)
	{
		if (!m_bIsCarried)
			return;
		
		// Store carrier info before clearing it
		IEntity previousCarrier = m_CurrentCarrier;
		
		// Detach from player
		DetachFromPlayer();
		
		// Move flag entity to drop position
		GetOwner().SetOrigin(position);
		
		// Notify gamemode
		if (m_CTFGamemode)
			m_CTFGamemode.FlagDropped(position);
		
		// Send drop notification
		SendFlagDropNotification(previousCarrier, position);
		
		m_CurrentCarrier = null;
		m_bIsCarried = false;
		
		Print("[CRF_CTFFlagComponent] Flag dropped at: " + position.ToString());
	}
	
	/**
	 * Drop flag at current carrier's position (used when player dies/disconnects)
	 */
	void DropFlagAtCarrierPosition()
	{
		if (!m_bIsCarried || !m_CurrentCarrier)
			return;
		
		// Get carrier's current position
		vector carrierPosition = m_CurrentCarrier.GetOrigin();
		
		// Add small offset to avoid flag spawning inside the body
		carrierPosition[1] = carrierPosition[1] + 0.5; // Lift 0.5m above ground
		
		// Store carrier info for notification
		IEntity previousCarrier = m_CurrentCarrier;
		
		// Detach from player
		DetachFromPlayer();
		
		// Move flag entity to drop position
		GetOwner().SetOrigin(carrierPosition);
		
		// Notify gamemode
		if (m_CTFGamemode)
			m_CTFGamemode.FlagDropped(carrierPosition);
		
		// Send special drop notification for death/disconnect
		SendFlagCarrierLostNotification(previousCarrier);
		
		m_CurrentCarrier = null;
		m_bIsCarried = false;
		
		Print("[CRF_CTFFlagComponent] Flag dropped due to carrier death/disconnect at: " + carrierPosition.ToString());
	}
	
	/**
	 * Reset flag to spawn position
	 */
	void ResetFlag()
	{
		// Store carrier info before clearing it for notification
		IEntity previousCarrier = m_CurrentCarrier;
		
		if (m_bIsCarried)
		{
			DetachFromPlayer();
		}
		
		m_CurrentCarrier = null;
		m_bIsCarried = false;
		
		// Notify gamemode
		if (m_CTFGamemode)
			m_CTFGamemode.ResetFlag();
		
		// Send reset notification
		SendFlagResetNotification(previousCarrier);
		
		Print("[CRF_CTFFlagComponent] Flag reset");
	}
	
	//===================================================================================
	// CARRIER MONITORING
	//===================================================================================
	
	/**
	 * Monitor the status of the flag carrier to handle death/disconnect
	 */
	protected void MonitorCarrierStatus()
	{
		// Only monitor if flag is currently carried
		if (!m_bIsCarried || !m_CurrentCarrier)
			return;
		
		// Check if carrier is still valid and alive
		if (!IsCarrierValid())
		{
			Print("[CRF_CTFFlagComponent] Carrier invalid or dead, dropping flag");
			DropFlagAtCarrierPosition();
		}
	}
	
	/**
	 * Check if the current flag carrier is still valid (alive and connected)
	 * @return True if carrier is valid and alive
	 */
	protected bool IsCarrierValid()
	{
		if (!m_CurrentCarrier)
			return false;
		
		// Check if it's a character entity
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_CurrentCarrier);
		if (!character)
			return false;
		
		// Check if character is alive
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(character.GetDamageManager());
		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			return false;
		
		// Check if player is still connected (for multiplayer)
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(m_CurrentCarrier);
			if (playerId <= 0)
				return false; // Player disconnected or no longer controlling this entity
		}
		
		return true;
	}
	
	/**
	 * Force drop flag if carrier becomes invalid (called externally)
	 */
	void HandleCarrierLost()
	{
		if (m_bIsCarried && m_CurrentCarrier)
		{
			DropFlagAtCarrierPosition();
		}
	}
	
	/**
	 * Handle when a specific player dies - check if they were carrying the flag
	 * @param deadPlayer The player who died
	 */
	void HandlePlayerDeath(IEntity deadPlayer)
	{
		if (!m_bIsCarried || !m_CurrentCarrier || !deadPlayer)
			return;
		
		// Check if the dead player was carrying the flag
		if (m_CurrentCarrier == deadPlayer)
		{
			Print("[CRF_CTFFlagComponent] Flag carrier died, dropping flag");
			DropFlagAtCarrierPosition();
		}
	}
	
	/**
	 * Handle when a player disconnects - check if they were carrying the flag
	 * @param disconnectedPlayerId The ID of the player who disconnected
	 */
	void HandlePlayerDisconnect(int disconnectedPlayerId)
	{
		if (!m_bIsCarried || !m_CurrentCarrier)
			return;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
		
		// Check if the disconnected player was carrying the flag
		int carrierId = playerManager.GetPlayerIdFromControlledEntity(m_CurrentCarrier);
		if (carrierId == disconnectedPlayerId)
		{
			Print("[CRF_CTFFlagComponent] Flag carrier disconnected, dropping flag");
			DropFlagAtCarrierPosition();
		}
	}
	
	//===================================================================================
	// CLEANUP
	//===================================================================================
	
	override void OnDelete(IEntity owner)
	{
		// Clean up monitoring when component is destroyed
		GetGame().GetCallqueue().Remove(MonitorCarrierStatus);
		super.OnDelete(owner);
	}
	
	//===================================================================================
	// NOTIFICATION SYSTEM
	//===================================================================================
	
	/**
	 * Send flag pickup notification to all clients
	 * @param player The player who picked up the flag
	 */
	protected void SendFlagPickupNotification(IEntity player)
	{
		if (!player)
			return;
		
		// Get player info
		string playerName = "";
		string factionKey = "";
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(player);
			if (playerId > 0)
				playerName = playerManager.GetPlayerName(playerId);
		}
		
		// Get player's faction
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(player);
		if (character)
			factionKey = character.GetFactionKey();
		
		// Fallback to entity name if player name not found
		if (playerName.IsEmpty())
			playerName = player.GetName();
		
		// Format the notification message
		string notificationText = string.Format("%1 (%2) has picked up the flag!", playerName, factionKey);
		
		// Send notification using vanilla Arma Reforger notification system
		// This will send to all players on the server
		Rpc(RPC_SendFlagNotificationToAll, notificationText);
		
		Print("[CRF_CTFFlagComponent] Sent flag pickup notification: " + notificationText);
	}
	
	/**
	 * Send flag drop notification to all clients
	 * @param previousCarrier The player who was carrying the flag
	 * @param dropPosition Where the flag was dropped
	 */
	protected void SendFlagDropNotification(IEntity previousCarrier, vector dropPosition)
	{
		string notificationText;
		
		if (previousCarrier)
		{
			// Get player info
			string playerName = "";
			string factionKey = "";
			
			PlayerManager playerManager = GetGame().GetPlayerManager();
			if (playerManager)
			{
				int playerId = playerManager.GetPlayerIdFromControlledEntity(previousCarrier);
				if (playerId > 0)
					playerName = playerManager.GetPlayerName(playerId);
			}
			
			// Get player's faction
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(previousCarrier);
			if (character)
				factionKey = character.GetFactionKey();
			
			// Fallback to entity name if player name not found
			if (playerName.IsEmpty())
				playerName = previousCarrier.GetName();
			
			notificationText = string.Format("%1 (%2) has dropped the flag!", playerName, factionKey);
		}
		else
		{
			notificationText = "The flag has been dropped!";
		}
		
		// Send notification using RPC to all clients
		Rpc(RPC_SendFlagNotificationToAll, notificationText);
		
		Print("[CRF_CTFFlagComponent] Sent flag drop notification: " + notificationText);
	}
	
	/**
	 * Send flag reset notification to all clients
	 * @param previousCarrier The player who was carrying the flag when it was reset
	 */
	protected void SendFlagResetNotification(IEntity previousCarrier)
	{
		string notificationText = "The flag has been reset to its spawn location!";
		
		// Send notification using RPC to all clients
		Rpc(RPC_SendFlagNotificationToAll, notificationText);
		
		Print("[CRF_CTFFlagComponent] Sent flag reset notification: " + notificationText);
	}
	
	/**
	 * Send flag carrier lost notification to all clients (death/disconnect)
	 * @param previousCarrier The player who was carrying the flag when they died/disconnected
	 */
	protected void SendFlagCarrierLostNotification(IEntity previousCarrier)
	{
		string notificationText;
		
		if (previousCarrier)
		{
			// Get player info
			string playerName = "";
			string factionKey = "";
			
			PlayerManager playerManager = GetGame().GetPlayerManager();
			if (playerManager)
			{
				int playerId = playerManager.GetPlayerIdFromControlledEntity(previousCarrier);
				if (playerId > 0)
					playerName = playerManager.GetPlayerName(playerId);
			}
			
			// Get player's faction
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(previousCarrier);
			if (character)
				factionKey = character.GetFactionKey();
			
			// Fallback to entity name if player name not found
			if (playerName.IsEmpty())
				playerName = previousCarrier.GetName();
			
			notificationText = string.Format("%1 (%2) died while carrying the flag! The flag has been dropped.", playerName, factionKey);
		}
		else
		{
			notificationText = "The flag carrier has been eliminated! The flag has been dropped.";
		}
		
		// Send notification using RPC to all clients
		Rpc(RPC_SendFlagNotificationToAll, notificationText);
		
		Print("[CRF_CTFFlagComponent] Sent flag carrier lost notification: " + notificationText);
	}
	
	//===================================================================================
	// RPC METHODS
	//===================================================================================
	
	/**
	 * RPC method to send notification to all clients
	 * @param message The notification message to display
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_SendFlagNotificationToAll(string message)
	{
		// Display notification using popup system
		SCR_PopUpNotification popupInstance = SCR_PopUpNotification.GetInstance();
		if (popupInstance)
		{
			// Use popup notification to display the message
			popupInstance.PopupMsg(message, duration: 5.0); // Show for 5 seconds
		}
		else
		{
			// Fallback: Print to chat if popup system not available
			Print("[CTF] " + message, LogLevel.NORMAL);
		}
	}
	
	//===================================================================================
	// VISUAL ATTACHMENT
	//===================================================================================
	
	/**
	 * Attach flag visually to the player
	 * @param player The player to attach the flag to
	 */
	protected void AttachToPlayer(IEntity player)
	{
		// Hide the flag entity (it will be shown as attachment on player)
		GetOwner().SetFlags(EntityFlags.VISIBLE, false);
		
		// TODO: Implement visual attachment to player model
		// This would involve attaching a flag mesh to the player's backpack attachment point
	}
	
	/**
	 * Detach flag from player
	 */
	protected void DetachFromPlayer()
	{
		// Show the flag entity again
		GetOwner().SetFlags(EntityFlags.VISIBLE, true);
		
		// TODO: Remove visual attachment from player model
	}
	
	//===================================================================================
	// GETTERS
	//===================================================================================
	
	/**
	 * Check if flag is currently being carried
	 * @return True if flag is carried
	 */
	bool IsCarried()
	{
		return m_bIsCarried;
	}
	
	/**
	 * Get current flag carrier
	 * @return The player entity carrying the flag, or null
	 */
	IEntity GetCarrier()
	{
		return m_CurrentCarrier;
	}
}