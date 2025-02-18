#include "common.hlsli"
#include "lmodel.hlsli"
#include "shadow.hlsli"

#ifndef USE_SUNMASK
float3x4 m_sunmask;
#endif

Texture2D s_water;

float4 main(float2 tc : TEXCOORD0, float2 tcJ : TEXCOORD1, float4 pos2d : SV_Position) : SV_Target
{
    gbuffer_data gbd = gbuffer_load_data(tc, pos2d);

    float4 _P = float4(gbd.P, 1.0);
    float4 PS = mul(m_shadow, _P);

    float s = shadow(PS);
    float2 tc1 = mul(m_sunmask, _P);
    tc1 /= 2;
    tc1 = frac(tc1);

    float4 water = s_water.SampleLevel(smp_linear, tc1, 0);

    water.xyz = (water.xzy - 0.5) * 2;
    water.xyz = mul(m_V, water.xyz);
    water *= s;

    return float4(water.xyz, s / 2);
}
