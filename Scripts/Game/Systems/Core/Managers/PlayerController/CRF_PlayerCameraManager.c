class CRF_PlayerCameraManagerClass : ScriptComponentClass
{
}

class CRF_PlayerCameraManager : ScriptComponent
{
	IEntity m_eCamera;                      // Stores local camera entity for spectator mode
	
	protected static CRF_PlayerCameraManager m_sInstance;
	
	protected bool m_bCameraOnRails;
	
	protected IEntity m_eCameraPlayer;
	protected vector m_vCameraOrbitPoint;
	protected float m_vCameraOrbitDistance;
	protected float m_vCameraOrbitHeight;
	protected PolylineShapeEntity m_CameraPolyLine;
	
	//------------------------------------------------------------------------------------------------
	// STATIC ACCESSORS
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the instance of this component from the player controller
	 * @return CRF_PlayerCameraManager - The camera manager component instance or null if unavailable
	 */
	
	void CRF_PlayerCameraManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_PlayerCameraManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	// METHODS
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void RemoveCamera()
	{
		delete m_eCamera;
	}
	
	//------------------------------------------------------------------------------------------------
	void RemoveCameraRails()
	{
		m_bCameraOnRails = false;
		
		// Level camera horizon
		vector mat = m_eCamera.GetAngles();
		m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsPlayer(IEntity player)
	{
		if (m_eCamera) {
			ClearCameraOnRailsVariables();
			m_eCameraPlayer = player;
			InitalizeCameraOnRails();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsOrbit(vector point, float distance, float height)
	{	
		if (m_eCamera) {
			ClearCameraOnRailsVariables();
			m_vCameraOrbitPoint = point;
			m_vCameraOrbitDistance = distance;
			m_vCameraOrbitHeight = height;
			InitalizeCameraOnRails();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsPolyline(PolylineShapeEntity poly)
	{
		if (m_eCamera) {
			ClearCameraOnRailsVariables();
			m_CameraPolyLine = poly;
			InitalizeCameraOnRails();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitalizeCameraOnRails()
	{
		m_bCameraOnRails = true;
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ClearCameraOnRailsVariables()
	{
		m_eCameraPlayer = null;
		m_CameraPolyLine = null;
		m_vCameraOrbitPoint = vector.Zero;
		m_vCameraOrbitDistance = 0;
		m_vCameraOrbitHeight = 0;
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	    super.EOnFrame(owner, timeSlice);
		
		if (!m_bCameraOnRails || !m_eCamera)
		{
			ClearCameraOnRailsVariables();
			ClearEventMask(owner, EntityEvent.FRAME);
			return;
		} else {
			switch (true) 
			{
				case (m_eCameraPlayer) : FrameUpdatePlayer(); break;
				case (m_vCameraOrbitPoint != vector.Zero) : FrameUpdateOrbit(); break;
				case (m_CameraPolyLine) : FrameUpdatePolyline(); break;
			}
		};
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	 * Initilizes players if they have a valid spectator entity
	 */
	void InitilizeSpecCamera()
	{
		if (!m_eCamera)
		{
			vector cameraPos[4];
			cameraPos = SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_vPlayersLastDeath;
			
			if (cameraPos[3] != vector.Zero) // Use provided death position if available
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			else // Fallback to generic spawn position
				cameraPos[3] = CRF_Gamemode.GetInstance().GetGenericSpawn();
				
			// Set up camera entity
			EntitySpawnParams cameraSpawnParams = new EntitySpawnParams();
			cameraSpawnParams.TransformMode = ETransformMode.WORLD;
			cameraSpawnParams.Transform = cameraPos;
	
			// Spawn or reposition camera
			m_eCamera = GetGame().SpawnEntityPrefabLocal(Resource.Load("{E1FF38EC8894C5F3}Prefabs/Systems/Editor/Camera/ManualCameraSpectate.et"), GetGame().GetWorld(), cameraSpawnParams);
			
			// Level camera horizon
			vector mat = m_eCamera.GetAngles();
			m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
		};
		
		// Switch to spectator camera
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
	};
	
	//------------------------------------------------------------------------------------------------
	protected void FrameUpdatePlayer()
	{
		// Get the slot component for camera positioning
		SlotManagerComponent slotComp = SlotManagerComponent.Cast(m_eCameraPlayer.FindComponent(SlotManagerComponent));
		if (!slotComp)
			return;
		
		// Get the first-person camera slot
		EntitySlotInfo cameraPoint = slotComp.GetSlotByName("CRF_FPP");
		if (!cameraPoint)
			return;
		
		// Get transform and modify it to be slightly behind and to the right of the player
		vector transform[4];
		cameraPoint.GetTransform(transform);
		
		// Calculate offset position (0.5m back, 0.3m right for over-shoulder view)
		vector offsetPosition = transform[3] - (transform[2] * 0.5) + (transform[0] * 0.3);
		transform[3] = offsetPosition;
		
		// Apply transform to spectator camera
		m_eCamera.SetTransform(transform);
	};
	
	//------------------------------------------------------------------------------------------------
	protected void FrameUpdateOrbit()
	{
	
	}
	
	//------------------------------------------------------------------------------------------------
	protected void FrameUpdatePolyline()
	{
	
	}
}