#ifndef COMMON_H
#define COMMON_H

// #define USE_SUPER_SPECULAR

#include "shared\common.hlsli"
//////////////////////////////////////////////////////////////////////////////////////////
// *** options

// #define DBG_TEST_NMAP
// #define DBG_TEST_NMAP_SPEC
// #define DBG_TEST_SPEC
// #define DBG_TEST_LIGHT
// #define DBG_TEST_LIGHT_SPEC

// #define USE_GAMMA_22
// #define USE_FETCH4
// #define USE_HWSMAP                	//- HW-options defined

// #define USE_HWSMAP_PCF				//- nVidia GF3+, R600+

// #define USE_BRANCHING        		//- HW-options defined
// #define USE_VTF                		//- HW-options defined, VertexTextureFetch
// #define FP16_FILTER                	//- HW-options defined
// #define FP16_BLEND                	//- HW-options defined
//
// #define USE_PARALLAX                	//- shader defined
// #define USE_TDETAIL                	//- shader defined
// #define USE_LM_HEMI                	//- shader defined
// #define USE_DISTORT                	//- shader defined
// #define DBG_TMAPPING
//////////////////////////////////////////////////////////////////////////////////////////
#ifndef SMAP_size
    #define SMAP_size 1024
#endif

#ifdef USE_R2_STATIC_SUN
    #define xmaterial float(1.0f / 4.0f)
#else
    #define xmaterial float(L_material.w)
#endif
//////////////////////////////////////////////////////////////////////////////////////////
uniform float4 def_aref;
uniform float4 parallax;
uniform float4 hemi_cube_pos_faces;
uniform float4 hemi_cube_neg_faces;
uniform float4 L_material;
uniform float4 Ldynamic_color;
uniform float4 Ldynamic_pos;
uniform float4 Ldynamic_dir;

uniform float4 J_direct[6];
uniform float4 J_spot[6];

float calc_fogging(float3 w_pos)
{
    return 1.0f - saturate(length(w_pos.xyz - eye_position.xyz) * fog_params.w + fog_params.x);
}

float2 calc_detail(float3 w_pos)
{
    float dtl = distance(w_pos, eye_position) * dt_params.w;
    dtl = min(dtl * dtl, 1.0f);
    float dt_mul = 1.0f - dtl; // dt*  [1 ..  0 ]
    float dt_add = 0.5f * dtl; // dt+  [0 .. 0.5]
    return float2(dt_mul, dt_add);
}
float3 calc_reflection(float3 pos_w, float3 norm_w)
{
    return reflect(normalize(pos_w - eye_position), norm_w);
}

float3 calc_sun_r1(float3 norm_w)
{
    return L_sun_color * saturate(dot((norm_w), -L_sun_dir_w));
}
float3 calc_model_hemi_r1(float3 norm_w)
{
    return max(0, norm_w.y) * L_hemi_color;
}
float3 calc_model_lq_lighting(float3 norm_w)
{
    return L_material.x * calc_model_hemi_r1(norm_w) + L_ambient + L_material.y * calc_sun_r1(norm_w);
}

//////////////////////////////////////////////////////////////////////////////////////////
struct v_static
{
    float4 P : POSITION; // (float,float,float,1)
    float4 Nh : NORMAL; // (nx,ny,nz,hemi occlusion)
    float4 T : TANGENT; // tangent
    float4 B : BINORMAL; // binormal
    float2 tc : TEXCOORD0; // (u,v)
    float2 lmh : TEXCOORD1; // (lmu,lmv)
    float4 color : COLOR0; // (r,g,b,dir-occlusion)
};

struct v_tree
{
    float4 P : POSITION; // (float,float,float,1)
    float4 Nh : NORMAL; // (nx,ny,nz)
    float3 T : TANGENT; // tangent
    float3 B : BINORMAL; // binormal
    float4 tc : TEXCOORD0; // (u,v,frac,???)
};

struct v_model
{
    float4 P : POSITION; // (float,float,float,1)
    float3 N : NORMAL; // (nx,ny,nz)
    float3 T : TANGENT; // (nx,ny,nz)
    float3 B : BINORMAL; // (nx,ny,nz)
    float2 tc : TEXCOORD0; // (u,v)
};

struct v_detail
{
    float4 pos : POSITION; // (float,float,float,1)
    int4 misc : TEXCOORD0; // (u(Q),v(Q),frac,matrix-id)
};

struct v_shadow_direct_aref
{
    float4 P : POSITION; // Clip-space position (for rasterization)
    float4 tc : TEXCOORD1; // Diffuse map for aref
};

struct p_bumped_new
{
    float4 hpos : POSITION;

    float4 tcdh : TEXCOORD0; // Texture coordinates, sun_occlusion || lm-hemi
    float4 position : TEXCOORD1; // position + hemi
    float3 M1 : TEXCOORD2; // nmap 2 eye - 1
    float3 M2 : TEXCOORD3; // nmap 2 eye - 2
    float3 M3 : TEXCOORD4; // nmap 2 eye - 3
};

//////////////////////////////////////////////////////////////////////////////////////////
struct p_bumped
{
    float4 hpos : POSITION;
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4 tcdh : TEXCOORD0; // Texture coordinates,         w=sun_occlusion
#else
    float2 tcdh : TEXCOORD0; // Texture coordinates
#endif
    float4 position : TEXCOORD1; // position + hemi
    float3 M1 : TEXCOORD2; // nmap 2 eye - 1
    float3 M2 : TEXCOORD3; // nmap 2 eye - 2
    float3 M3 : TEXCOORD4; // nmap 2 eye - 3
#ifdef USE_TDETAIL
    float2 tcdbump : TEXCOORD5; // d-bump
    #ifdef USE_LM_HEMI
    float2 lmh : TEXCOORD6; // lm-hemi
    #endif
#else
    #ifdef USE_LM_HEMI
    float2 lmh : TEXCOORD5; // lm-hemi
    #endif
#endif
};
//////////////////////////////////////////////////////////////////////////////////////////
struct p_flat
{
    float4 hpos : POSITION;
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
    float4 tcdh : TEXCOORD0; // Texture coordinates,         w=sun_occlusion
#else
    float2 tcdh : TEXCOORD0; // Texture coordinates
#endif
    float4 position : TEXCOORD1; // position + hemi
    float3 N : TEXCOORD2; // Eye-space normal        (for lighting)
#ifdef USE_TDETAIL
    float2 tcdbump : TEXCOORD3; // d-bump
    #ifdef USE_LM_HEMI
    float2 lmh : TEXCOORD4; // lm-hemi
    #endif
#else
    #ifdef USE_LM_HEMI
    float2 lmh : TEXCOORD3; // lm-hemi
    #endif
#endif
};

//////////////////////////////////////////////////////////////////////////////////////////
// struct                  f_deffer        		{
// float4           position        		: COLOR0;        // px,py,pz, m-id
// float4           Ne                		: COLOR1;        // nx,ny,nz, hemi
// float4       	C                		: COLOR2;        // r, g, b,  gloss
// };

struct f_deffer
{
    float4 P : COLOR0;
    float4 N : COLOR1;
    float4 C : COLOR2;
};

struct f_forward
{
    float4 Color : COLOR0;
};

struct p_shadow
{
    float2 tc0 : TEXCOORD0;
    float4 hpos : POSITION;
};
//////////////////////////////////////////////////////////////////////////////////////////
struct p_screen
{
    float4 hpos : POSITION;
    float2 tc0 : TEXCOORD0;
};
//////////////////////////////////////////////////////////////////////////////////////////
// Geometry phase / deferring               	//
uniform sampler2D s_base; //
uniform sampler2D s_bump; //
uniform sampler2D s_bumpX; //
uniform sampler2D s_detail; //
uniform sampler2D s_detailBump; //
uniform sampler2D s_detailBumpX; //	Error for bump detail
uniform sampler2D s_bumpD; //
uniform sampler2D s_hemi; //

uniform sampler2D s_mask; //

uniform sampler2D s_dt_r; //
uniform sampler2D s_dt_g; //
uniform sampler2D s_dt_b; //
uniform sampler2D s_dt_a; //

uniform sampler2D s_dn_r; //
uniform sampler2D s_dn_g; //
uniform sampler2D s_dn_b; //
uniform sampler2D s_dn_a; //

//////////////////////////////////////////////////////////////////////////////////////////
// Lighting/shadowing phase                     //
uniform sampler2D s_depth; //
uniform sampler2D s_position; //
uniform sampler2D s_normal; //
uniform sampler s_lmap; // 2D/cube projector lightmap
uniform sampler3D s_material; //
uniform sampler1D s_attenuate; //
//////////////////////////////////////////////////////////////////////////////////////////
// Combine phase                                //
uniform sampler2D s_diffuse; // rgb.a = diffuse.gloss
uniform sampler2D s_accumulator; // rgb.a = diffuse.specular
uniform sampler2D s_generic; //
uniform sampler2D s_bloom; //
uniform sampler s_image; // used in various post-processing
uniform sampler2D s_tonemap; // actually MidleGray / exp(Lw + eps)

#define def_gloss float(4.0f / 255.0f)
#define def_dbumph float(0.333f)
#define def_virtualh float(0.05f)
#define def_distort float(0.05f)
#define def_hdr float(9.0f)
#define def_hdr_clip float(0.75f)
#define LUMINANCE_VECTOR float3(0.3f, 0.38f, 0.22f)

float3 tonemap(float3 rgb, float scale)
{
    rgb = rgb * scale;

    const float fWhiteIntensity = 1.7f;
    const float fWhiteIntensitySQR = fWhiteIntensity * fWhiteIntensity;

    return rgb * (1.0f + rgb / fWhiteIntensitySQR) / (rgb + 1.0f);
}

float4 combine_bloom(float3 low, float4 high)
{
    return float4(low + high * high.a, 1.0f);
}

float3 v_hemi(float3 n)
{
    return L_hemi_color * (.5f + .5f * n.y);
}
float3 v_hemi_wrap(float3 n, float w)
{
    return L_hemi_color * (w + (1 - w) * n.y);
}
float3 v_sun(float3 n)
{
    return L_sun_color * dot(n, -L_sun_dir_w);
}
float3 v_sun_wrap(float3 n, float w)
{
    return L_sun_color * (w + (1 - w) * dot(n, -L_sun_dir_w));
}
float3 p_hemi(float2 tc)
{
    //        float3        	t_lmh         = tex2D             	(s_hemi, tc);
    //        return  dot     (t_lmh,1.h/4.h);
    float4 t_lmh = tex2D(s_hemi, tc);
    return t_lmh.a;
}

float get_hemi(float4 lmh)
{
    return lmh.a;
}

float get_sun(float4 lmh)
{
    return lmh.g;
}

//	contrast function
float Contrast(float Input, float ContrastPower)
{
    // piecewise contrast function
    bool IsAbovefloat = Input > 0.5f;
    float ToRaise = saturate(2.0f * (IsAbovefloat ? 1.0f - Input : Input));
    float Output = 0.5f * pow(ToRaise, ContrastPower);
    Output = IsAbovefloat ? 1.0f - Output : Output;
    return Output;
}

f_deffer pack_gbuffer(float4 Normal, float4 Point, float4 Color)
{
    f_deffer Output;
    Output.N = Normal;
    Output.P = Point;
    Output.C = Color;
    return Output;
}

#define FXPS \
    technique _render \
    { \
        pass _code \
        { \
            PixelShader = compile ps_3_0 main(); \
        } \
    }
#define FXVS \
    technique _render \
    { \
        pass _code \
        { \
            VertexShader = compile vs_3_0 main(); \
        } \
    }

#endif
