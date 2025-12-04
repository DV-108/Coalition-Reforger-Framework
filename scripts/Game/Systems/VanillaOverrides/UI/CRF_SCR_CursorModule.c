[BaseContainerProps()]
modded class SCR_MapCursorModule
{
	override protected void HandleSelect()
	{
		if (m_MapEntity)
		{
			if (m_MapEntity.m_bCreatingFlight)
			{
				m_MapEntity.m_iPhase++;
				switch (m_MapEntity.m_iPhase)
				{
					case 0:
					break;
					
					case 1:
					float wX, wY;
					m_MapEntity.GetMapCursorWorldPosition(wX, wY);
					m_MapEntity.m_vFlightStart = Vector(wX, 0, wY);
					break;
					
					case 2:
					float wX, wY;
					m_MapEntity.GetMapCursorWorldPosition(wX, wY);
					m_MapEntity.m_vFlightEnd = Vector(wX, 0, wY);
					break;
					
					case 3:
					break;
				}
			}
		}
		super.HandleSelect();
	}
}