#include "../xrLC/StdAfx.h"
#include "../xrLC/Build.h"
#include "../xrLC_Light/xrLC_GlobalData.h"

CBuild* pBuild = NULL;
u32		version = 0;

static const char* h_str =
"The following keys are supported / required:\n"
"-? or -h	== this help\n"
"-o			== modify build options\n"
"-nosun		== disable sun-lighting\n"
"-skipinvalid\t== skip crash if invalid faces exists\n"
"-notess	== don`t use tesselate geometry\n"
"-nosubd	== don`t use subdivide geometry\n"
"-tex_rgba	== don`t compress lightmap textures\n"
"-f<NAME>	== compile level in GameData\\Levels\\<NAME>\\\n"
"-skip_weld == don't use welding vertexis \n"
"-lmaps_alt == ���������� �������������� ����� ligtmaps building texture \n"
"-lmaps_size == ������ ������ ������� LMAP\n"
"-use_intel == ���������� �������������� ����� Raytracing Intel Embree \n"

"\n"
"NOTE: The last key is required for any functionality\n";

void Help(const char*);

typedef int __cdecl xrOptions(b_params* params, u32 version, bool bRunBuild);
extern bool g_using_smooth_groups;

#include "CompilersUI.h"

extern CompilersMode gCompilerMode;;

void StartupLC()
{
	// 	xr_strcpy(cmd, lpCmdLine);
	//	_strlwr(cmd);


	g_build_options.b_radiosity = gCompilerMode.LC_GI;
	g_build_options.b_noise = gCompilerMode.LC_Noise;
	g_using_smooth_groups = !gCompilerMode.LC_NoSMG;

	// Faster FPU 
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	// Load project
	for (auto& [Name, Selected] : gCompilerMode.Files)
	{
		if (!Selected)
			continue;

		create_global_data();
		lc_global_data()->b_nosun_set(gCompilerMode.LC_NoSun);
		lc_global_data()->SetSkipInvalid(gCompilerMode.LC_SkipInvalidFaces);
		lc_global_data()->SetSkipTesselate(!gCompilerMode.LC_Tess);
		lc_global_data()->SetLmapRGBA(gCompilerMode.LC_tex_rgba);
		lc_global_data()->SetSkipSubdivide(gCompilerMode.LC_NoSubdivide);

		// Se7kills
		lc_global_data()->SetIsIntelUse(gCompilerMode.Embree);
		lc_global_data()->SetSkipWeld(gCompilerMode.LC_skipWeld);
		lc_global_data()->SetLmapsSize(gCompilerMode.LC_sizeLmaps);

		lc_global_data()->SetLevelName(Name.data());

		string256 temp;
		xr_sprintf(temp, "%s - Levels Compiler", Name.data());
		SDL_SetWindowTitle(g_AppInfo.Window, temp);

		string_path prjName;
		FS.update_path(prjName, "$game_levels$", xr_strconcat(prjName, Name.data(), "\\build.prj"));

		string256 phaseName;
		Phase(xr_strconcat(phaseName, "Reading project [", Name.data(), "]..."));

		string256 inf;
		IReader* F = FS.r_open(prjName);
		if (NULL == F) 
		{
			xr_sprintf(inf, "Build failed!\nCan't find level: '%s'", Name.data());
			clMsg(inf);
			MessageBoxA(nullptr, inf, "Error!", MB_OK | MB_ICONERROR);
			return;
		}

		// Version
		F->r_chunk(EB_Version, &version);
		clMsg("version: %d", version);
		R_ASSERT(XRCL_CURRENT_VERSION == version);

		// Header
		b_params Params;
		F->r_chunk(EB_Parameters, &Params);

		// Conversion
		Phase("Converting data structures...");
		pBuild = new CBuild();
		pBuild->Load(Params, *F);

		lc_global_data()->SetOverrideSettings(gCompilerMode.IsOverloadedSettings);

		if (gCompilerMode.IsOverloadedSettings)
		{
			g_params().m_lm_jitter_samples = gCompilerMode.LC_JSample;
			g_params().m_lm_pixels_per_meter = gCompilerMode.LC_Pixels;
			
			clMsg("Original Weld Distance: %f", g_params().m_weld_distance);
			g_params().m_weld_distance = gCompilerMode.MergeDistance;

			clMsg("Original m_sm_angle: %f", g_params().m_sm_angle);

			lc_global_data()->SetJitterMU(gCompilerMode.LC_JSampleMU);
		}

		FS.r_close(F);

		// Call for builder
		string_path lfn;
		FS.update_path(lfn, _game_levels_, Name.data());
		pBuild->Run(lfn);
		xr_delete(pBuild);
	}
}