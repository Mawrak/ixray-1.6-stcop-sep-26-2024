#include "common.hlsli"
#include "sload.hlsli"

struct p_particle
{
    float4 color : COLOR0;
    p_flat base;
};

float4 main(p_particle II) : SV_Target0
{
    discard;
    return 0.0f;
}

