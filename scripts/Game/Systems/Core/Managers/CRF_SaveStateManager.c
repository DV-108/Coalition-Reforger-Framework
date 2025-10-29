class CRF_SaveStateManagerClass: SCR_BaseGameModeComponentClass
{
}

class CRF_SaveStateManager: SCR_BaseGameModeComponent
{
	protected CRF_SlottingManager m_SlottingManager;
	private string m_sLastFileName = "";
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
	float m_fTimeBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		m_fTimeBuffer += timeSlice;
		if (m_fTimeBuffer > 10)
		{
			m_fTimeBuffer = 0;
			SaveSessionState();
		}
	}
	
	void SaveSessionState()
	{
		array<ref CRF_SlotDataContainer> slots = m_SlottingManager.GetSlotDataArray();
		SCR_JsonSaveContext state = new SCR_JsonSaveContext;
		int day, month, year;
		System.GetYearMonthDay(year, month, day);
		int hour, minute, second;
		System.GetHourMinuteSecond(hour, minute, second);
		string saveTime = string.Format("%1%2_%3.%4.%5", month, day, hour, minute, second);
		state.WriteValue("MissionName", GetGame().GetMissionName());
		state.WriteValue("SaveTime", saveTime);
		
		array<ref CRF_SaveStateSlotData> saveSlots = {};
		
		foreach (CRF_SlotDataContainer slot: slots)
		{
			if (slot.GetSlotCurrentPlayerId() <= 0)
				continue;
			
			ref CRF_SaveStateSlotData newSlot = new CRF_SaveStateSlotData();
			SCR_AIGroup group = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(slot.GetSlotCurrentGroup())).GetEntity());
			string company, platoon, squad, character, format;
			group.GetCallsigns(company, platoon, squad, character, format);
			newSlot.m_sGroupName = squad;
			newSlot.m_iGroupSlotId = slot.m_iSlotGroupId;
			newSlot.m_sSlotGUID = slot.GetSlotCurrentGUID();
			newSlot.m_bDeathState = slot.GetIsDeadSlot();
			newSlot.m_sFactionKey = slot.GetSlotFactionKey();
			if (!newSlot.m_bDeathState && slot.GetSlotCurrentCharacter() != RplId.Invalid())
			{
				if (Replication.FindItem(slot.GetSlotCurrentCharacter()))
				{
					IEntity entity = RplComponent.Cast(Replication.FindItem(slot.GetSlotCurrentCharacter())).GetEntity();
					if (entity)
					{
						vector transform[4];
						entity.GetTransform(transform);
						newSlot.m_vTransform1 = transform[0];
						newSlot.m_vTransform2 = transform[1];
						newSlot.m_vTransform3 = transform[2];
						newSlot.m_vTransform4 = transform[3];
					}
				}
			}
			saveSlots.Insert(newSlot);
		}
		state.WriteValue("Slots", saveSlots);
		if (!FileIO.FileExists("$profile:/SessionSave"))
			FileIO.MakeDirectory("$profile:/SessionSave");
		string fileName = "$profile:/SessionSave/" + GetGame().GetMissionName() + saveTime + ".json";
		Print(m_sLastFileName);
//		if (m_sLastFileName != "")
//			FileIO.DeleteFile(m_sLastFileName);
		
		if (m_sLastFileName == "")
			LoadSaveState(fileName);
		
		m_sLastFileName = fileName;
		state.SaveToFile(fileName);
	}
	
	void LoadSaveState(string file)
	{
		SCR_JsonLoadContext loadState = new SCR_JsonLoadContext();
		loadState.LoadFromFile("$profile:/SessionSave/1028_23.19.15.json");
		array<ref CRF_SaveStateSlotData> loadedSlots = {};
		loadState.ReadValue("Slots", loadedSlots);
		array<ref CRF_SlotDataContainer> slots = m_SlottingManager.GetSlotDataArray();
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		map<int, string> playerGUIDMap = new map<int, string>;
		foreach (int playerId: players)
		{
			playerGUIDMap.Insert(playerId, GetGame().GetBackendApi().GetPlayerIdentityId(playerId));
		}
		foreach (CRF_SaveStateSlotData newSlot: loadedSlots)
		{
			int index = -1;
			foreach (CRF_SlotDataContainer slot: slots)
			{
				index++;
				SCR_AIGroup group = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(slot.GetSlotCurrentGroup())).GetEntity());
				string company, platoon, squad, character, format;
				group.GetCallsigns(company, platoon, squad, character, format);
				Print(squad);
				
				if (slot.GetSlotFactionKey() != newSlot.m_sFactionKey)
					continue;
				
				if (squad != newSlot.m_sGroupName)
					continue;
				
				if (newSlot.m_iGroupSlotId != slot.m_iSlotGroupId)
					continue;
				
				int playerId = 0;
				foreach(int player, string guid: playerGUIDMap)
				{
					if (guid == newSlot.m_sSlotGUID)
						playerId = player;
				}
	
				m_SlottingManager.BatchUpdateSlot(m_SlottingManager.GetSlotKeys().Get(index), playerId);
				slot.SetSlotCurrentGUID(newSlot.m_sSlotGUID);
				m_SlottingManager.GetSlotsWaitingArray().Insert(slot);
			}
		}
	}
}

class CRF_SaveStateSlotData
{
	string m_sGroupName;
	int m_iGroupSlotId;
	string m_sSlotGUID;
	bool m_bDeathState;
	string m_sFactionKey;
	vector m_vTransform1;
	vector m_vTransform2;
	vector m_vTransform3;
	vector m_vTransform4;
}