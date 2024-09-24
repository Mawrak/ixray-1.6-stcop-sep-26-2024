#pragma once

class CAgentManager;
class FRbmkAgentManagerPlanner
{
public:
					FRbmkAgentManagerPlanner	(CAgentManager *InOwner);
					~FRbmkAgentManagerPlanner	();
	void			Update						();
private:
	bool			IsItemSelected				() const;
	bool			IsEnemySelected				() const;
	bool			IsDangerSelected			() const;
	void			RefreshState					(int32 NewState);
	int32			CurrentState = -1;
	CAgentManager*	Owner;
};
