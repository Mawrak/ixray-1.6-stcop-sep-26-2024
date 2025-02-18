#include "common.hlsli"

#ifndef ISAMPLE
    #define ISAMPLE 0
#endif

// Pixel
//	TODO: DX10: move to load instead of sample (will need to provide integer texture coordinates)
float4 main(float4 tc : TEXCOORD0) : SV_Target
{
    // return	tex2Dproj	(s_base,tc);

    //	Perform texture coordinates projection.
    tc.xy /= tc.w;
    return s_generic.Sample(smp_nofilter, tc);
}
