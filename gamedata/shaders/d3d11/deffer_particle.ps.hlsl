#include "common.hlsli"
#include "sload.hlsli"

struct p_particle
{
    float4 color : COLOR0;
    p_flat base;
};

float4 main(p_particle II) : SV_Target0
{
    // f_deffer	O;
    // p_flat	I;

    // I=II.base;

    // // 1. Base texture + kill pixels with low alpha
    // float4 D = s_base.Sample(smp_base, I.tcdh);
    // D	*=	II.color;
    // clip(D.w-def_aref);

    // // 2. Standart output
    // float4		Ne  = float4	(normalize((float3)I.N.xyz)					, I.position.w	);
    // O				= pack_gbuffer(
    // Ne,
    // float4 	(I.position.xyz + Ne.xyz*def_virtualh / 2.0f, xmaterial		),
    // float4	(D.xyz,			def_gloss) );		// OUT: rgb.gloss
    // O.V = I.hpos_curr.xy / I.hpos_curr.w - I.hpos_old.xy / I.hpos_old.w;
    // return O;

    discard;
    return 0.0f;
}
