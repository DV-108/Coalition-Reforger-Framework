modded class R3D_PIPComponent
{
	override void EOnInit(IEntity owner)
    {
		if (SCR_Global.IsEditMode())
			return;
        
		WorkspaceWidget workspace = GetGame().GetWorkspace();
        Widget root = workspace.CreateWidgets(m_sLayoutPath);
		if (!root)
		{
			Print("R3D_PIPComponent | Unable to create layout", LogLevel.ERROR);
			return;
		} 
		
		ScriptCamera m_Camera = CreateCamera(owner, lastIndx);
		if (!m_Camera)
		{
			Print("R3D_PIPComponent | Couldn't create the camera entity", LogLevel.ERROR);
			return;
		}
		
        m_renderTargetTexture = RTTextureWidget.Cast(workspace.CreateWidgets(m_sRenderTargetPrefabPath, root));
		m_renderTargetTexture.SetName(string.Format("RT%1", lastIndx));
		Print(m_renderTargetTexture);
		if (!m_renderTargetTexture)
		{
			Print("R3D_PIPComponent | Couldn't find RT Texture widget", LogLevel.ERROR);
			return;
		} 
		
		//RenderTargetWidget m_rtWidget = RenderTargetWidget.Cast(root.FindAnyWidget("RenderTarget0"));
		m_rtWidget = RenderTargetWidget.Cast(m_renderTargetTexture.FindAnyWidget("RenderTarget0"));
		m_rtWidget.SetName(string.Format("RenderTarget%1", lastIndx));
		m_rtWidget.SetRefresh(3,0);
		m_rtWidget.SetResolutionScale(0.1, 0.1);
		Print(m_rtWidget);
		if (!m_rtWidget)
		{
			Print("R3D_PIPComponent | Couldn't find RenderTarget widget", LogLevel.ERROR);
			return;
		} 
		
		idx = lastIndx;
		
		m_rtWidget.SetWorld(GetGame().GetWorld(), lastIndx);
        //m_renderTargetTexture.SetGUIWidget(owner, lastIndx);
		lastIndx++;
    }   
}