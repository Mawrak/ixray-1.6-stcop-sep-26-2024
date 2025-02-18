#include "common.hlsli"

#define SMAA_HLSL_3

uniform float4 screen_res;
#define SMAA_RT_METRICS screen_res.zwxy

#define SMAA_PRESET_ULTRA
#define EDGE_DETECT_COLOR

#include "smaa.hlsli"

// Struct
struct p_smaa
{
    float2 tc0 : TEXCOORD0; // Texture coordinates         (for sampling maps)
};

float4 main(p_smaa I) : COLOR
{
    float4 offset[3];
    SMAAEdgeDetectionVS(I.tc0, offset);

#if defined(EDGE_DETECT_COLOR)
    return float4(SMAAColorEdgeDetectionPS(I.tc0, offset, s_image), 0.0f, 0.0f);
#else
    return float4(SMAALumaEdgeDetectionPS(I.tc0, offset, s_image), 0.0f, 0.0f);
#endif
}
