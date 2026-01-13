/*
* FPS Telemetry Manager
* Collects FPS data from all clients every 10 minutes and logs to file
* Server-side only
*/

[ComponentEditorProps(category: "CRF FPS Telemetry", description: "Collects and logs client FPS data for performance monitoring")]
class CRF_FPSTelemetryManagerClass : SCR_BaseGameModeComponentClass
{
}

//------------------------------------------------------------------------------------------------
// FPS data container for individual players
class CRF_PlayerFPSData
{
	int m_iPlayerId;
	string m_sPlayerName;
	float m_fCurrentFPS;
	float m_fAverageFPS;
	float m_fMinFPS;
	float m_fMaxFPS;
	int m_iSampleCount;
	float m_fLastReportTime;
	
	void CRF_PlayerFPSData(int playerId, string playerName, float fps)
	{
		m_iPlayerId = playerId;
		m_sPlayerName = playerName;
		m_fCurrentFPS = fps;
		m_fAverageFPS = fps;
		m_fMinFPS = fps;
		m_fMaxFPS = fps;
		m_iSampleCount = 1;
		m_fLastReportTime = System.GetTickCount();
	}
	
	void UpdateFPS(float fps)
	{
		m_fCurrentFPS = fps;
		m_fLastReportTime = System.GetTickCount();
		
		// Update running average
		m_fAverageFPS = ((m_fAverageFPS * m_iSampleCount) + fps) / (m_iSampleCount + 1);
		m_iSampleCount++;
		
		// Update min/max
		if (fps < m_fMinFPS)
			m_fMinFPS = fps;
		if (fps > m_fMaxFPS)
			m_fMaxFPS = fps;
	}
	
	void ResetStats()
	{
		m_fAverageFPS = m_fCurrentFPS;
		m_fMinFPS = m_fCurrentFPS;
		m_fMaxFPS = m_fCurrentFPS;
		m_iSampleCount = 1;
	}
}

//------------------------------------------------------------------------------------------------
class CRF_FPSTelemetryManager : SCR_BaseGameModeComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Enable FPS telemetry logging")]
	protected bool m_bEnableTelemetry;
	
	[Attribute("600", UIWidgets.EditBox, "Interval in seconds to collect and log FPS data (default: 600 = 10 minutes)")]
	protected int m_iCollectionInterval;
	
	[Attribute("1", UIWidgets.CheckBox, "Save FPS logs to file")]
	protected bool m_bSaveToFile;
	
	[Attribute("$profile:FPSTelemetry", UIWidgets.EditBox, "Directory path for FPS log files")]
	protected string m_sLogDirectory;
	
	protected static CRF_FPSTelemetryManager s_Instance;
	protected ref map<int, ref CRF_PlayerFPSData> m_mPlayerFPSData = new map<int, ref CRF_PlayerFPSData>();
	protected float m_fLastCollectionTime;
	protected float m_fSessionStartTime;
	protected int m_iLogFileCounter;
	
	//------------------------------------------------------------------------------------------------
	void CRF_FPSTelemetryManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
		m_fLastCollectionTime = System.GetTickCount();
		m_fSessionStartTime = m_fLastCollectionTime;
		m_iLogFileCounter = 0;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_FPSTelemetryManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		if (m_bEnableTelemetry)
		{
			Print("[CRF_FPSTelemetry] FPS Telemetry system initialized", LogLevel.NORMAL);
			Print(string.Format("[CRF_FPSTelemetry] Collection interval: %1 seconds (%2 minutes)", 
				m_iCollectionInterval, m_iCollectionInterval / 60), LogLevel.NORMAL);
			Print(string.Format("[CRF_FPSTelemetry] Save to file: %1", m_bSaveToFile), LogLevel.NORMAL);
			if (m_bSaveToFile)
				Print(string.Format("[CRF_FPSTelemetry] Log directory: %1", m_sLogDirectory), LogLevel.NORMAL);
			
			SetEventMask(owner, EntityEvent.FRAME);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		
		// Check if we should collect and log FPS data
		float currentTime = System.GetTickCount();
		if (currentTime - m_fLastCollectionTime >= m_iCollectionInterval * 1000)
		{
			CollectAndLogFPSData();
			m_fLastCollectionTime = currentTime;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Receive FPS report from client via RPC
	void ReportClientFPS(int playerId, float fps, string playerName)
	{
		// Only run on server
		if (!Replication.IsServer() || !m_bEnableTelemetry)
			return;
		
		CRF_PlayerFPSData data = m_mPlayerFPSData.Get(playerId);
		if (!data)
		{
			// First report from this player
			data = new CRF_PlayerFPSData(playerId, playerName, fps);
			m_mPlayerFPSData.Set(playerId, data);
			Print(string.Format("[CRF_FPSTelemetry] First FPS report from player: %1 (ID: %2) - FPS: %3", 
				playerName, playerId, Math.Round(fps)), LogLevel.VERBOSE);
		}
		else
		{
			// Update existing player data
			data.UpdateFPS(fps);
			data.m_sPlayerName = playerName; // Update name in case it changed
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Collect FPS data and log to console and file
	protected void CollectAndLogFPSData()
	{
		if (m_mPlayerFPSData.Count() == 0)
			return;
		
		// Remove disconnected players
		CleanupDisconnectedPlayers();
		
		if (m_mPlayerFPSData.Count() == 0)
			return;
		
		// Print to console
		PrintFPSSummary();
		
		// Save to file if enabled
		if (m_bSaveToFile)
			SaveFPSDataToFile();
		
		// Reset stats for next interval
		foreach (int playerId, CRF_PlayerFPSData data : m_mPlayerFPSData)
		{
			data.ResetStats();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Remove players who are no longer connected
	protected void CleanupDisconnectedPlayers()
	{
		array<int> disconnectedPlayers = new array<int>();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		foreach (int playerId, CRF_PlayerFPSData data : m_mPlayerFPSData)
		{
			if (!playerManager || !playerManager.IsPlayerConnected(playerId))
			{
				disconnectedPlayers.Insert(playerId);
			}
		}
		
		foreach (int playerId : disconnectedPlayers)
		{
			m_mPlayerFPSData.Remove(playerId);
			Print(string.Format("[CRF_FPSTelemetry] Removed disconnected player ID: %1", playerId), LogLevel.VERBOSE);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Print FPS summary to console
	protected void PrintFPSSummary()
	{
		float sessionTime = (System.GetTickCount() - m_fSessionStartTime) / 1000.0;
		
		Print("========================================", LogLevel.NORMAL);
		Print("[CRF_FPSTelemetry] Client FPS Summary", LogLevel.NORMAL);
		Print(string.Format("Session Time: %1 minutes", Math.Round(sessionTime / 60.0)), LogLevel.NORMAL);
		Print(string.Format("Active Players: %1", m_mPlayerFPSData.Count()), LogLevel.NORMAL);
		Print("========================================", LogLevel.NORMAL);
		Print("Player Name                     ID      Current    Average    Min        Max        Samples", LogLevel.NORMAL);
		Print("----------------------------------------", LogLevel.NORMAL);
		
		// Calculate server averages
		float totalAvgFPS = 0;
		float lowestFPS = 999999;
		float highestFPS = 0;
		
		foreach (int playerId, CRF_PlayerFPSData data : m_mPlayerFPSData)
		{
			// Pad player name to 30 characters
			string paddedName = data.m_sPlayerName;
			while (paddedName.Length() < 30)
				paddedName += " ";
			if (paddedName.Length() > 30)
				paddedName = paddedName.Substring(0, 30);
			
			Print(string.Format("%1 %2 %3 %4 %5 %6 %7",
				paddedName,
				data.m_iPlayerId.ToString(-1, 7),
				Math.Round(data.m_fCurrentFPS).ToString(-1, 10),
				Math.Round(data.m_fAverageFPS).ToString(-1, 10),
				Math.Round(data.m_fMinFPS).ToString(-1, 10),
				Math.Round(data.m_fMaxFPS).ToString(-1, 10),
				data.m_iSampleCount.ToString()), LogLevel.NORMAL);
			
			totalAvgFPS += data.m_fAverageFPS;
			if (data.m_fMinFPS < lowestFPS)
				lowestFPS = data.m_fMinFPS;
			if (data.m_fMaxFPS > highestFPS)
				highestFPS = data.m_fMaxFPS;
		}
		
		Print("----------------------------------------", LogLevel.NORMAL);
		Print(string.Format("Server Average FPS: %1", Math.Round(totalAvgFPS / m_mPlayerFPSData.Count())), LogLevel.NORMAL);
		Print(string.Format("Lowest FPS Recorded: %1", Math.Round(lowestFPS)), LogLevel.NORMAL);
		Print(string.Format("Highest FPS Recorded: %1", Math.Round(highestFPS)), LogLevel.NORMAL);
		Print("========================================", LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	// Save FPS data to file
	protected void SaveFPSDataToFile()
	{
		// Generate filename with timestamp
		int year, month, day, hour, minute, second;
		System.GetYearMonthDay(year, month, day);
		System.GetHourMinuteSecond(hour, minute, second);
		
		string timestamp = string.Format("%1-%2-%3_%4-%5-%6", 
			year, 
			month.ToString(2), 
			day.ToString(2), 
			hour.ToString(2), 
			minute.ToString(2), 
			second.ToString(2));
		
		m_iLogFileCounter++;
		string filename = string.Format("FPS_Log_%1_Session%2.csv", timestamp, m_iLogFileCounter);
		string fullPath = m_sLogDirectory + "/" + filename;
		
		// Create directory if it doesn't exist
		if (!FileIO.MakeDirectory(m_sLogDirectory))
		{
			Print(string.Format("[CRF_FPSTelemetry] Warning: Could not create directory: %1", m_sLogDirectory), LogLevel.WARNING);
		}
		
		// Open file for writing
		FileHandle file = FileIO.OpenFile(fullPath, FileMode.WRITE);
		if (!file)
		{
			Print(string.Format("[CRF_FPSTelemetry] Error: Could not open file for writing: %1", fullPath), LogLevel.ERROR);
			return;
		}
		
		// Write CSV header
		file.WriteLine("Timestamp,PlayerID,PlayerName,CurrentFPS,AverageFPS,MinFPS,MaxFPS,SampleCount");
		
		// Write player data
		foreach (int playerId, CRF_PlayerFPSData data : m_mPlayerFPSData)
		{
			string line = string.Format("%1,%2,\"%3\",%4,%5,%6,%7,%8",
				timestamp,
				data.m_iPlayerId,
				data.m_sPlayerName,
				data.m_fCurrentFPS,
				data.m_fAverageFPS,
				data.m_fMinFPS,
				data.m_fMaxFPS,
				data.m_iSampleCount);
			
			file.WriteLine(line);
		}
		
		// Close file
		file.Close();
		
		Print(string.Format("[CRF_FPSTelemetry] FPS data saved to: %1", fullPath), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	// Get player FPS data (useful for admin tools)
	CRF_PlayerFPSData GetPlayerFPSData(int playerId)
	{
		return m_mPlayerFPSData.Get(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	// Get all player FPS data
	map<int, ref CRF_PlayerFPSData> GetAllPlayerFPSData()
	{
		return m_mPlayerFPSData;
	}
	
	//------------------------------------------------------------------------------------------------
	// Reset telemetry data
	void ResetTelemetry()
	{
		if (!Replication.IsServer())
			return;
			
		m_mPlayerFPSData.Clear();
		m_fLastCollectionTime = System.GetTickCount();
		m_fSessionStartTime = m_fLastCollectionTime;
		m_iLogFileCounter = 0;
		
		Print("[CRF_FPSTelemetry] FPS telemetry data reset", LogLevel.NORMAL);
	}
}
