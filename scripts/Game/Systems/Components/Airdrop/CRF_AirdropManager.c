class CRF_AirdropManagerClass: SCR_BaseGameModeComponentClass
{
}

class CRF_AirdropManager: SCR_BaseGameModeComponent
{
	static CRF_AirdropManager m_sInstance;
	protected ref array<ref CRF_AirdropFlightObject> m_aFlights = {};
	ref array<int> m_aAssignedSquads = {};
	ref array<ref CRF_Flightpath> m_aFlightpaths = {};
	
	void CRF_AirdropManager (IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_AirdropManager GetInstance()
	{
		return m_sInstance;
	}
	
	void InitFlight(CRF_AirdropObject planeObject)
	{
		SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
		array<SCR_AIGroup> groups = {};
		groupsMan.GetAllPlayableGroups(groups);
		int planeIndex = 0;
		array<string> playersInPlane = {""};
		int playersAdded = 0;
		PlayerManager pm = GetGame().GetPlayerManager();
		foreach (SCR_AIGroup group: groups)
		{
			if (!group.GetFaction())
				continue;
			
			if (group.GetFaction().GetFactionKey() != planeObject.m_sFactionKey)
				continue;
			
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			
			if (agents.Count() == 0)
				continue;
			
			//This group will put us past the 40 slots available in the plane, we gotta spawn another one
			if (playersAdded + agents.Count() > 40)
			{
				planeIndex++;
				playersAdded = 0;
				playersInPlane.Insert("");
			}
			foreach (AIAgent agent: agents)
			{
				int playerId = pm.GetPlayerIdFromControlledEntity(agent.GetControlledEntity());
				if (playerId <= 0)
					continue;
				
				string currentPlayers = playersInPlane.Get(planeIndex);
				currentPlayers += playerId.ToString() + "|";
				playersInPlane.Set(planeIndex, currentPlayers);
				playersAdded++;
			}
			
		}
		
		for (int i = 0; i <= planeIndex; i++)
		{
			GetGame().GetCallqueue().CallLater(SpawnFlight, 5000 * i, false, planeObject, playersInPlane.Get(i));
		}
	}
	
	void SpawnFlight(CRF_AirdropObject planeObject, string players)
	{
		vector angles[3];
		Math3D.AnglesToMatrix(Vector(planeObject.m_fAngle, 0, 0), angles);
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[0] = angles[0];
		params.Transform[1] = angles[1];
		params.Transform[2] = angles[2];
		params.Transform[3] = planeObject.m_vFlightCoordinates[0];
		IEntity plane = GetGame().SpawnEntityPrefab(Resource.Load(planeObject.m_sPlane), null, params);
		ref CRF_AirdropFlightObject flight = new CRF_AirdropFlightObject(plane, planeObject.m_vFlightCoordinates, 65);
		//Delay so the flight has a chance to actual load the entity
		//Ensure the entity has also been streamed in for all players as well
		GetGame().GetCallqueue().CallLater(TeleportPlayers, 2000, false, players, SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent)), plane, flight);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PlaySound(RplId planeId)
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		if (plane.FindComponent(SCR_BaseInteractiveLightComponent))
			SCR_BaseInteractiveLightComponent.Cast(plane.FindComponent(SCR_BaseInteractiveLightComponent)).ToggleLight(true);
	}
	
	void TeleportPlayers(string players, SlotManagerComponent slotMan, IEntity plane, CRF_AirdropFlightObject flight)
	{
		array<string> playerIds = {};
		players.Split("|", playerIds, true);
		int slotId = 0;
		RplId planeRplId = RplComponent.Cast(plane.FindComponent(RplComponent)).Id();
		PlayerManager pm = GetGame().GetPlayerManager();
		foreach (int i, string playerId: playerIds)
		{
			//Let's delay adding them until the player has had time to teleport into the plane
			GetGame().GetCallqueue().CallLater(flight.m_PlayersInPlane.Insert, 2000, false, pm.GetPlayerControlledEntity(playerId.ToInt()));
			EntitySlotInfo slot = slotMan.GetSlotByName("Slot" + slotId.ToString());
			vector transform[4];
			slot.GetLocalTransform(transform);
			if (i < 20)
	        {
	            transform[3][2] = transform[3][2] - (0.8 * i);
	        }
	        else
	        {
	            transform[3][0] = transform[3][0] + 1.4;
	            transform[3][2] = transform[3][2] - (0.8 * (i - 20));
	        }
			vector pos = plane.CoordToParent(transform[3]);
			transform[3] = pos;
			SCR_Global.TeleportPlayer(playerId.ToInt(), transform[3], SCR_EPlayerTeleportedReason.NONE);
			Rpc(RpcDo_TeleportPlayer, playerId.ToInt(), planeRplId, slotId, i);
		}
		RpcDo_PlaySound(planeRplId);
		Rpc(RpcDo_PlaySound, planeRplId);
		RedLight(planeRplId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TeleportPlayer(int playerId, RplId planeId, int slotId, int index)
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		SlotManagerComponent slotMan = SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent));
		EntitySlotInfo slot = slotMan.GetSlotByName("Slot" + slotId);
		vector transform[4];
		slot.GetLocalTransform(transform);
		if (index < 20)
        {
            transform[3][2] = transform[3][2] - (0.8 * index);
        }
        else
        {
            transform[3][0] = transform[3][0] + 1.4;
            transform[3][2] = transform[3][2] - (0.8 * (index - 20));
        }
		vector pos = plane.CoordToParent(transform[3]);
		transform[3] = pos;
		SCR_Global.TeleportPlayer(playerId, transform[3], SCR_EPlayerTeleportedReason.NONE);
	}
	
	float m_fParachuteCheck = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		if (!m_aFlights)
		{
			ClearEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
			return;
		}
		
		if (m_aFlights.Count() == 0)
		{
			ClearEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
			return;
		}
		
		bool checkDeployParachutes = false;
		if (m_fParachuteCheck >= 0.1)
		{
			checkDeployParachutes = true;
			m_fParachuteCheck = 0;
		}
		else
			m_fParachuteCheck += timeSlice;
		foreach (CRF_AirdropFlightObject flight: m_aFlights)
		{
			
			if (checkDeployParachutes)
			{
				PlayerManager pm = GetGame().GetPlayerManager();
				foreach (int i, IEntity player: flight.m_PlayersInPlane)
				{
					//Opens shoot for players more than 200m away
					if (vector.Distance(player.GetOrigin(), flight.m_Plane.GetOrigin()) > 50)
					{
						int playerId = pm.GetPlayerIdFromControlledEntity(player);
						if (playerId <= 0)
							continue;
						
						ParachuteComponent.Cast(pm.GetPlayerController(playerId).FindComponent(ParachuteComponent)).RpcAskDeployParachute();
						flight.m_PlayersInPlane.Remove(i);
					}
				}
			}		
			
			if (flight.m_fProgress >= 2.0)
            	m_aFlights.RemoveItem(flight);

			float distance = vector.Distance(flight.m_vFlightCoordinates[0], flight.m_vFlightCoordinates[3]);
			float step = (flight.m_fSpeed * timeSlice) / distance;
			
			vector A = flight.m_vFlightCoordinates[0]; 
			vector B = flight.m_vFlightCoordinates[3]; 
			vector G = flight.m_vFlightCoordinates[2]; 
			
			vector AB = B - A;
			vector AG = G - A;
			
			float lenAB = AB.Length();
			flight.m_fGreenT = vector.Dot(AG, AB) / (lenAB * lenAB);
			flight.m_fGreenT = Math.Clamp(flight.m_fGreenT, 0, 1);

    		float previousProgress = flight.m_fProgress;
			flight.m_fProgress = Math.Clamp(flight.m_fProgress + step, 0, 2);
			
			if (previousProgress < flight.m_fGreenT && flight.m_fProgress >= flight.m_fGreenT)
			    GreenLight(flight.m_RplId);
	
	        vector newPos = vector.Lerp(flight.m_vFlightCoordinates[0], flight.m_vFlightCoordinates[3], flight.m_fProgress);
			vector transform[4];
			flight.m_Plane.GetTransform(transform);
			transform[3] = newPos;
			
			GenericEntity plane = GenericEntity.Cast(flight.m_Plane);
	        plane.SetWorldTransform(transform);
			plane.Update();
			plane.OnTransformReset();
			Rpc(RpcDo_BroadcastPositionUpdate, flight.m_RplId, transform);
		}
	}
	
	void RedLight(RplId planeId)
	{
		Rpc(RpcDo_SpawnRedLight, planeId);
	}
	
	void GreenLight(RplId planeId)
	{
		Rpc(RpcDo_SpawnGreenLight, planeId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SpawnRedLight(RplId planeId)
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		SlotManagerComponent slotMan = SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent));
		
		for (int i = 0; i < 5; i++)
		{
			EntitySpawnParams params = new EntitySpawnParams();
			EntitySlotInfo slot = slotMan.GetSlotByName("LightSlot" + i.ToString());
			if (slot.GetAttachedEntity())
				delete slot.GetAttachedEntity();
			
			slot.GetWorldTransform(params.Transform);
			IEntity light = GetGame().SpawnEntityPrefab(Resource.Load("{22E711A4236A5FE4}PrefabsEditable/Auto/Props/RedLight.et"), null, params);
			slot.AttachEntity(light);
		}
		
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SpawnGreenLight(RplId planeId)
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		SlotManagerComponent slotMan = SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent));
		
		for (int i = 0; i < 5; i++)
		{
			EntitySpawnParams params = new EntitySpawnParams();
			EntitySlotInfo slot = slotMan.GetSlotByName("LightSlot" + i.ToString());
			if (slot.GetAttachedEntity())
				delete slot.GetAttachedEntity();
			
			slot.GetWorldTransform(params.Transform);
			IEntity light = GetGame().SpawnEntityPrefab(Resource.Load("{134538EF4DD59327}PrefabsEditable/Auto/Props/GreenLight.et"), null, params);
			slot.AttachEntity(light);
		}
		
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastPositionUpdate(RplId planeId, vector transform[4])
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		GenericEntity gPlane = GenericEntity.Cast(plane);
        gPlane.SetWorldTransform(transform);
		gPlane.Update();
		gPlane.OnTransformReset();
	}
	
	void RegisterFlight(CRF_AirdropFlightObject flight)
	{
		m_aFlights.Insert(flight);
		SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
	}
	
	void RequestAirdropUIUpdate()
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		pc.RequestAirdropUIUpdate();
	}
	
	array<int> GetSideAssignedSquad(string factionKey)
	{
		array<int> groups = {};
		SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsMan) // oh fuck
			return groups;
	
		foreach (int groupId: m_aAssignedSquads)
		{
			SCR_AIGroup group = groupsMan.FindGroup(groupId);
			if (!group)
				continue;
			
			Faction groupFaction = group.GetFaction();
			if (!groupFaction)
				continue;
			
			if (groupFaction.GetFactionKey() != factionKey)
				continue;
			
			
		}
		return groups;
	}
	
	array<CRF_Flightpath> GetSideFlightPaths(string factionKey)
	{
		array<CRF_Flightpath> flightPaths = {};
		foreach (CRF_Flightpath flightPath: m_aFlightpaths)
		{
			if (flightPath.m_sFactionKey == factionKey)
				flightPaths.Insert(flightPath);
		}
		return flightPaths;
	}
}

class CRF_AirdropFlightObject
{
	void CRF_AirdropFlightObject(IEntity plane, vector flightCoordinates[4], float speed)
	{
		m_Plane = plane;
		m_RplId = RplComponent.Cast(plane.FindComponent(RplComponent)).Id();
		m_vFlightCoordinates = flightCoordinates;
		m_fSpeed = speed;
		CRF_AirdropManager.GetInstance().RegisterFlight(this);
	}
	
	void ~CRF_AirdropFlightObject()
	{
		if (!Replication.IsServer())
			return;
		
		SCR_EntityHelper.DeleteEntityAndChildren(m_Plane);
	}
	
	IEntity m_Plane;
	ref array<IEntity> m_PlayersInPlane = {};
	RplId m_RplId;
	vector m_vFlightCoordinates[4];
	bool m_bGreenLight = false;
	float m_fProgress = 0;
	float m_fSpeed;
	float m_fGreenT;
}

//For UI
class CRF_AirdropFlight
{
	array<int> m_aAssignedGroups = {};
	ref CRF_Flightpath m_FlightPath;
	int m_iAltitude;
}

class CRF_AirdropObject
{
	void CRF_AirdropObject(string factionKey, ResourceName resourceName, vector flightCoordinates[4], float angle)
	{
		m_sFactionKey = factionKey;
		m_sPlane = resourceName;
		m_vFlightCoordinates = flightCoordinates;
		m_fAngle = angle;
	}
	string m_sFactionKey;
	ResourceName m_sPlane;
	float m_fAngle;
	//0 - Redlight start
	//1 - Redlight end
	//2 - Greenlight start
	//3 - Greenlight end
	vector m_vFlightCoordinates[4];
}

class CRF_Flightpath
{
	string m_sFactionKey;
	vector m_vStartPoint;
	vector m_vEndPoint;
	vector m_vGreenLight;
	
	
	static bool Extract(CRF_Flightpath instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeString(instance.m_sFactionKey);
		snapshot.SerializeBytes(instance.m_vStartPoint, 12);
		snapshot.SerializeBytes(instance.m_vEndPoint, 12);
		snapshot.SerializeBytes(instance.m_vGreenLight, 12);
		
		return true;
	}
	
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, CRF_Flightpath instance)
	{
		snapshot.SerializeString(instance.m_sFactionKey);
	    snapshot.SerializeBytes(instance.m_vStartPoint, 12);
	    snapshot.SerializeBytes(instance.m_vEndPoint, 12);
	    snapshot.SerializeBytes(instance.m_vGreenLight, 12);
		
		return true;
	}
	
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeString(packet);
	    snapshot.EncodeVector(packet);
		snapshot.EncodeVector(packet);
		snapshot.EncodeVector(packet);
	}
	
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeString(packet);
	    snapshot.DecodeVector(packet);
		snapshot.DecodeVector(packet);
		snapshot.DecodeVector(packet);
		
		return true;
	}
	
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
	    return lhs.CompareStringSnapshots(rhs)
			&& lhs.CompareSnapshots(rhs, 12)
	        && lhs.CompareSnapshots(rhs, 12)
	        && lhs.CompareSnapshots(rhs, 12);
	}	
	
	static bool PropCompare(CRF_Flightpath instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
	    return snapshot.CompareString(instance.m_sFactionKey)
			&& snapshot.Compare(instance.m_vStartPoint, 12)
	        && snapshot.Compare(instance.m_vEndPoint, 12)
	        && snapshot.Compare(instance.m_vGreenLight, 12);
	}	
}