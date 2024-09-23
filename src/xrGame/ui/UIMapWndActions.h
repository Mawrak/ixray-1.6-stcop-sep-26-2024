#pragma once
#include "UIMapWnd.h"

class FRbmkMapActionPlanner
{

public:
					FRbmkMapActionPlanner	(CUIMapWnd* Owner);
	void			Update();
	void			Reset();
private:
	CUIMapWnd*		Owner;
	int32			CurrentState = 0;
	
	float			EndMovingTime;
	float			TargetZoom;
	Frect			DesiredMapRect;
};

