class CRF_EntityHelper
{	
	//------------------------------------------------------------------------------------------------
	// Helper method to get group from RplId
	SCR_AIGroup GetGroupFromRplId(RplId groupId)
	{
		if (groupId == RplId.Invalid())
			return null;
			
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(groupId));
		if (!rplComp)
			return null;
			
		return SCR_AIGroup.Cast(rplComp.GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to get character from RplId
	static SCR_ChimeraCharacter GetCharacterFromRplId(RplId charId)
	{
		if (charId == RplId.Invalid())
			return null;
			
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(charId));
		if (!rplComp)
			return null;
			
		return SCR_ChimeraCharacter.Cast(rplComp.GetEntity());
	}
}