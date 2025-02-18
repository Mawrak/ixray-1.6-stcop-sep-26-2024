#include "common.hlsli"

struct v2p
{
    float2 tc0 : TEXCOORD0; // base
    float3 tc1 : TEXCOORD1; // environment
    float4 tc2 : TEXCOORD2; // lmap
    float3 c0 : COLOR0; // sun
    float4 c1 : COLOR1; // lq-color + factor
    float fog : FOG;
};

// Pixel
float4 main(v2p I) : COLOR
{
    float4 t_base = tex2D(s_base, I.tc0);
    float4 t_env = texCUBE(s_env, I.tc1);
    float4 t_lmap = tex2Dproj(s_lmap, I.tc2);

    // lighting
    float3 l_base = t_lmap.rgb; // base light-map (lmap color, ambient, hemi, etc - inside)
    float3 l_sun = I.c0 * t_lmap.a; // sun color
    float3 light = lerp(l_base + l_sun, I.c1, I.c1.w);

    // final-color
    float3 base = lerp(t_env, t_base, t_base.a);
    float3 final = light * base * 2.0f;
    final = lerp(fog_color.xyz, final, I.fog);

    // out
    return float4(final.r, final.g, final.b, t_base.a);
}
