
#include "stdafx.h"
#include "build.h"

#include "xrPhase_MergeLM_Rect.h"
#include "../xrLC_Light/xrdeflector.h"
#include "../xrLC_Light/xrlc_globaldata.h"
#include "../xrLC_Light/lightmap.h"

class	pred_remove { public: IC bool	operator() (CDeflector* D) { { if (0 == D) return TRUE; }; if (D->bMerged) { D->bMerged = FALSE; return TRUE; } else return FALSE; }; };

IC int	compare_defl(CDeflector* D1, CDeflector* D2)
{
	// First  - by material
	u16 M1 = D1->GetBaseMaterial();
	u16 M2 = D2->GetBaseMaterial();
	if (M1 < M2)	return	1;  // less
	if (M1 > M2)	return	0;	// more
	return				2;	// equal
}

// should define LESS(D1<D2) behaviour
// sorting - in increasing order
IC int	sort_defl_analyze(CDeflector* D1, CDeflector* D2)
{
	// first  - get material index
	u16 M1 = D1->GetBaseMaterial();
	u16 M2 = D2->GetBaseMaterial();

	// 1. material area
	u32	 A1 = pBuild->materials()[M1].internal_max_area;
	u32	 A2 = pBuild->materials()[M2].internal_max_area;
	if (A1 < A2)	return	2;	// A2 better
	if (A1 > A2)	return	1;	// A1 better

	// 2. material sector (geom - locality)
	u32	 s1 = pBuild->materials()[M1].sector;
	u32	 s2 = pBuild->materials()[M2].sector;
	if (s1 < s2)	return	2;	// s2 better
	if (s1 > s2)	return	1;	// s1 better

	// 3. just material index
	if (M1 < M2)	return	2;	// s2 better
	if (M1 > M2)	return	1;	// s1 better

	// 4. deflector area
	u32 da1 = D1->layer.Area();
	u32 da2 = D2->layer.Area();
	if (da1 < da2)return	2;	// s2 better
	if (da1 > da2)return	1;	// s1 better

	// 5. they are EQUAL
	return				0;	// equal
}

// should define LESS(D1<D2) behaviour
// sorting - in increasing order
IC bool	sort_defl_complex(CDeflector* D1, CDeflector* D2)
{
	switch (sort_defl_analyze(D1, D2))
	{
		case 1:		return true;	// 1st is better 
		case 2:		return false;	// 2nd is better
		case 0:		return false;	// none is better
		default:	return false;
	}
}

void MergeLmap(vecDefl& Layer, CLightmap* lmap, int& MERGED)
{
	// Process 	
	int _X = 0, _Y = 0;
 	u16 _Max_y = 0;

#define SHIFT_HEIGHT 1

	for (int it = 0; it < Layer.size(); it++)
	{
 		if (0 == (it % 1024))
			StatusNoMsg("Process [%d/%d]...Merged{%d}", it, g_XSplit.size(), MERGED);

		if (_Y > getLMSIZE())
  			break;

		lm_layer& L = Layer[it]->layer;
 
		u32 WIDTH = L.width + (2 * BORDER);
		u32 HEIGHT = L.height + (2 * BORDER);

		if (_Max_y < HEIGHT)
			_Max_y = HEIGHT;

		if (_X + WIDTH > getLMSIZE() - 32 )
		{
			_X = 0;
			_Y += _Max_y + SHIFT_HEIGHT;
			_Max_y = 0;
		}

		{
			L_rect		rT, rS;
			rS.a.set(_X, _Y);
			rS.b.set(_X + WIDTH, _Y + HEIGHT);
			rS.iArea = L.Area();
			rT = rS;

			// ����� ������ � ������������ LMerge
			BOOL		bRotated = false;  

			if (_Y < getLMSIZE() - HEIGHT)
			{
				lmap->Capture(Layer[it], rT.a.x, rT.a.y, rT.SizeX(), rT.SizeY(), bRotated);
				Layer[it]->bMerged = TRUE;
				MERGED++;
			}

			_X += WIDTH + SHIFT_HEIGHT;
		}
	 
		

		Progress(float(it) / float(g_XSplit.size()));
	}
}
 
void CBuild::xrPhase_MergeLM()
{
	R_ASSERT(lc_global_data());
	R_ASSERT(pBuild);

	string512	phase_name;
	xr_sprintf(phase_name, "Building lightmaps ...");
	Phase(phase_name);
	
	vecDefl			Layer;

	// **** Select all deflectors, which contain this light-layer
	Layer.clear();
	for (u32 it = 0; it < lc_global_data()->g_deflectors().size(); it++)
	{
		CDeflector* D = lc_global_data()->g_deflectors()[it];
		if (D->bMerged)	
			continue;
		Layer.push_back(D);
	}
	
	CTimer tSort; tSort.Start();
	// ��������� 1 ��� !!!! (����������� �� ������� ������)
	std::stable_sort(Layer.begin(), Layer.end(), [&](CDeflector* D1, CDeflector* D2)
	{
		if (D1->layer.height < D2->layer.height)
			return true;
		else
			return false;
	}); 

	clMsg("Sorting Lmaps time %u ms", tSort.GetElapsed_ms());
 	AditionalData("Sorting Lmaps time %u ms", tSort.GetElapsed_ms());

	// Merge this layer (which left unmerged)
	u32 StartSize = Layer.size();
	u32 TotalMerged = 0;
	 
	while (Layer.size())
	{
		// Allocation CLightMap (1k * 1k) (2k * 2k) (4k * 4k) (8k * 8k)  PIXELS !!!! 
		CLightmap* lmap = new CLightmap();
 		lc_global_data()->lightmaps().push_back(lmap);

		int MERGED = 0;
		MergeLmap(Layer, lmap, MERGED);
 		TotalMerged += MERGED;

		Progress ( float( float(MERGED) /  float( StartSize ) ) );
 
		// Remove merged lightmaps
 		vecDeflIt last = std::remove_if(Layer.begin(), Layer.end(), pred_remove());
		Layer.erase(last, Layer.end());


		CTimer t;  t.Start();
		// Save
 		lmap->Save(pBuild->path);
 		clMsg( "Saving Map: %u ms, Merged: %u/%u, WaitingMerge: %u, ", t.GetElapsed_ms(), MERGED, TotalMerged, Layer.size());
 		AditionalData("save: %ums| (Merged: %u/tot:%u | ToMerge:%u)", t.GetElapsed_ms(), MERGED, TotalMerged, Layer.size());
	}

	VERIFY(lc_global_data());
	clMsg("%d lightmaps builded", lc_global_data()->lightmaps().size());

	size_t used, free, reserved;
	vminfo(&free, &reserved, &used);

	clMsg("Start Destroy Deflectors: Memory: %llu mb used", u32 (used / 1024 / 1024) );

	// Cleanup deflectors
	Progress(1.f);
	Status("Destroying deflectors...");
	for (u32 it = 0; it < lc_global_data()->g_deflectors().size(); it++)
		xr_delete(lc_global_data()->g_deflectors()[it]);


	lc_global_data()->g_deflectors().clear();
 	vminfo(&free, &reserved, &used);

	clMsg("End Destroy Deflectors: Memory: %llu mb used", u32(used / 1024 / 1024));

	size_t USED_MEMORY = 0;
	for (auto LM : lc_global_data()->lightmaps())
	{
		USED_MEMORY += LM->lm.memory_lmap();
	}

	u32 USED_LMAPS = USED_MEMORY / 1024 / 1024;
	clMsg("Allocated FOR Lmaps Memory: %u mb", u32(USED_LMAPS) );

	AditionalData("Lmaps allocated: %u mb", USED_LMAPS);
}
