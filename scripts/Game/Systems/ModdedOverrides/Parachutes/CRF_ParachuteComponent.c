modded class ParachuteComponent
{
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override void RpcAskDeployParachute() // ← new parameter
	{
		if (m_bParachuteDeployed || !m_ParachuteItem || !pilotEntity || m_ParachuteItem.GetParachuteUsed())
			return;

		/* spawn with setup TRUE so components self‑insert */
		EntitySpawnParams p = new EntitySpawnParams;
		p.TransformMode = ETransformMode.WORLD;
		pilotEntity.GetWorldTransform(p.Transform);

		IEntity parachuteEntity = GetGame().SpawnEntityPrefabEx(
			m_ParachuteItem.GetParachutePrefab(),
			/*setup*/ false,
			GetGame().GetWorld(),
			p);
		m_DeployedParachute = ParachuteDeployedEntity.Cast(parachuteEntity);

		giveOwnershipToClient();

		vector v0 = pilotEntity.GetPhysics().GetVelocity();

		GetGame().GetCallqueue().CallLater(RpcAskSyncReplication, 100, false, m_DeployedParachute.GetRplId());
		GetGame().GetCallqueue().CallLater(RpcAskSyncVelocity, 150, false, v0);

		m_bParachuteDeployed = true;
	}
}