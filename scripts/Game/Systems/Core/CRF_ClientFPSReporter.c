/*
* Client FPS Reporter
* Periodically collects and reports local FPS to server
* Client-side only
*/

//------------------------------------------------------------------------------------------------
class CRF_ClientFPSReporter
{
	protected static ref CRF_ClientFPSReporter s_Instance;
	protected float m_fLastReportTime;
	protected int m_fReportInterval = 30000; // 30 seconds (in milliseconds)
	protected bool m_bEnabled = true;
	
	//------------------------------------------------------------------------------------------------
	static CRF_ClientFPSReporter GetInstance()
	{
		if (!s_Instance)
			s_Instance = new CRF_ClientFPSReporter();
		
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	void CRF_ClientFPSReporter()
	{
		m_fLastReportTime = System.GetTickCount();
	}
	
	//------------------------------------------------------------------------------------------------
	// Check if it's time to report FPS
	void Update()
	{
		// Only run on client
		if (Replication.IsServer())
			return;
		
		if (!m_bEnabled)
			return;
		
		float currentTime = System.GetTickCount();
		if (currentTime - m_fLastReportTime >= m_fReportInterval)
		{
			ReportFPS();
			m_fLastReportTime = currentTime;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Collect and send FPS data to server
	protected void ReportFPS()
	{
		// Get local FPS
		float currentFPS = System.GetFPS();
		
		// Get player information
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;
		
		int playerId = playerController.GetPlayerId();
		
		// Get player name
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
		
		string playerName = playerManager.GetPlayerName(playerId);
		if (!playerName || playerName.IsEmpty())
			playerName = "Unknown";
		
		// Send FPS data to server via RPC
		CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
		if (rplManager)
		{
			rplManager.ReportClientFPS(playerId, currentFPS, playerName);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Enable/disable FPS reporting
	void SetEnabled(bool enabled)
	{
		m_bEnabled = enabled;
	}
	
	//------------------------------------------------------------------------------------------------
	// Set report interval in seconds
	void SetReportInterval(int seconds)
	{
		m_fReportInterval = seconds * 1000;
	}
}
