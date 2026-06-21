#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "Program.hpp"
#include <string>

namespace gpu
{
    // Structs
    struct VertexPositionInputs
    {
        float3 positionWS; // World space position
        float3 positionVS; // View space position
        float4 positionCS; // Homogeneous clip space position
        float4 positionNDC;// Homogeneous normalized device coordinates
    };

    struct VertexNormalInputs
    {
        real3 tangentWS;
        real3 bitangentWS;
        float3 normalWS;
    };

    struct SurfaceData
    {
        half3 albedo;
        half3 specular;
        half  metallic;
        half  smoothness;
        half3 normalTS;
        half3 emission;
        half  occlusion;
        half  alpha;
        half  clearCoatMask;
        half  clearCoatSmoothness;
    };


    struct InputData
    {
        float3  positionWS;
        float4  positionCS;
        float3   normalWS;
        half3   viewDirectionWS;
        float4  shadowCoord;
        half    fogCoord;
        half3   vertexLighting;
        half3   bakedGI;
        float2  normalizedScreenSpaceUV;
        half4   shadowMask;
        half3x3 tangentToWorld;
    };

    // Abstraction over Light shading data.
    struct Light
    {
        half3   direction;
        half3   color;
        float   distanceAttenuation; // full-float precision required on some platforms
        half    shadowAttenuation;
        uint    layerMask;
    };


    float4 _ProjectionParams;
    float4 _ScreenParams;
    float4 _ZBufferParams;
    float4 unity_FogParams;
    float4 _ScaledScreenParams;
    float3 _WorldSpaceCameraPos;

    // scaleBias.x = flipSign
    // scaleBias.y = scale
    // scaleBias.z = bias
    // scaleBias.w = unused
    float4 _ScaleBias;
    float4 _ScaleBiasRt;

    float4 _BaseMap_ST;

    float4x4 UNITY_MATRIX_M;

    float4x4 UNITY_MATRIX_I_M;

    float4x4 UNITY_PREV_MATRIX_M;

    float4x4 UNITY_PREV_MATRIX_I_M;

    float4x4 UNITY_MATRIX_V;

    float4x4 UNITY_MATRIX_VP;

    float4x4 UNITY_MATRIX_P;

    // Per-material baked lightmap scale (approximates missing baked GI; default=1).
    // Ignored when _LIGHTMAP is true (real lightmap overrides this).
    half3 _BakedGIColor = half3(1, 1, 1);

    // Baked lightmap (per-object). Null when no lightmap was exported for the object.
    // _LightmapST = (scaleX, scaleY, offsetX, offsetY) mapping UV2 → lightmap UVs.
    Texture2D*   _Lightmap    = nullptr;
    SamplerState sampler_Lightmap;
    float4       _LightmapST  = float4(1, 1, 0, 0);
    bool         _LIGHTMAP    = false;
    // Unity HDR lightmaps are RGBM-encoded: final = rgb * (alpha * multiplier).
    // When the exporter wrote raw RGBM bytes (older exports), the runtime must
    // decode them. Newer exports pre-decode in the exporter; set this false then.
    // multiplier is unity_Lightmap_HDR.y (default 8 in URP).
    bool        _LIGHTMAP_RGBM_DECODE = true;
    float       _LIGHTMAP_RGBM_MULT   = 3.6f; // tune: 8 (Unity default) over-bright; 3.6 matches this scene
    // Global scale applied to sampled baked-GI radiance (tuning; Unity defaults to 1).
    half        _LIGHTMAP_INTENSITY   = 1.0f;

    // Flat fallback ambient (used when SH data is absent).
    half3 _AmbientColor = half3(0, 0, 0);

    // L2 Spherical Harmonics ambient probe (A2).
    // Layout: sh[c*9 + i], c=0R/1G/2B, i=0..8 (L0+L1+L2 basis indices).
    // Uploaded once per frame from scene.json "environment.sh" (27 floats).
    float _SH9[27]    = {};
    bool  _SH9_VALID  = false;

    // Indirect specular (A3): fallback flat colour when no cubemap probe is available.
    // Set to the SH L0 average when SH is uploaded; zero otherwise.
    half4 _GlossyEnvironmentColor = half4(0, 0, 0, 1);

    // HDR decode parameters for the specCube (A3).
    // (1,1,0,0) = linear pass-through so DecodeHDREnvironment is a no-op multiplier.
    // Set once at startup; overridden when a real cubemap probe is loaded.
    float4 unity_SpecCube0_HDR = float4(1, 1, 0, 0);

    //这个实际上是主光源的方向
    float4 _MainLightPosition;

    half4 _MainLightColor(1,1,1,1);
    half4 _MainLightOcclusionProbes;
    uint _MainLightLayerMask = 1;
    half4 unity_LightData(0,0,0,0);
    half4 unity_LightIndices[2]; //(光源数据相关, 光源总数、光源偏移量、光源索引)

    // Additional point/spot lights (D1).
    // _AdditionalLightsColor stores pre-multiplied color * intensity.
    // _AdditionalLightsPosition stores world-space XYZ + 1/range^2 in W (for attenuation).
    // _AdditionalLightsSpotDir stores spot direction XYZ + cos(halfOuterAngle) in W.
    // _AdditionalLightsAttenuation stores (cos(halfInner)-cos(halfOuter)) falloff ramp in X.
    static constexpr int MAX_ADDITIONAL_LIGHTS = 16;
    int    _AdditionalLightsCount = 0;
    float4 _AdditionalLightsPosition  [MAX_ADDITIONAL_LIGHTS];
    half4  _AdditionalLightsColor      [MAX_ADDITIONAL_LIGHTS];
    half4  _AdditionalLightsSpotDir    [MAX_ADDITIONAL_LIGHTS];
    float4 _AdditionalLightsAttenuation[MAX_ADDITIONAL_LIGHTS];

    bool _ALPHAPREMULTIPLY_ON = false;
    bool _SPECGLOSSMAP = false;
    bool _SPECULAR_COLOR = false;
    bool _GLOSSINESS_FROM_BASE_ALPHA = false;

    Texture2D* _BaseMap = nullptr;
    SamplerState sampler_BaseMap;
    Texture2D* _SpecGlossMap = nullptr;
    SamplerState sampler_SpecGlossMap;
    Texture2D* _BumpMap = nullptr;
    SamplerState sampler_BumpMap;
    Texture2D* _EmissionMap = nullptr;
    SamplerState sampler_EmissionMap;
    Texture2D* _OcclusionMap = nullptr;
    SamplerState sampler_OcclusionMap;
    half _OcclusionStrength = 1.0f;
    bool _OCCLUSIONMAP = false;
    half _BumpScale = 1.0f;
    // URP _SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A: smoothness from albedo.a vs mask.a.
    bool _SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A = false;

    // Texture filtering: bilinear by default (closer to Unity); point is kept as
    // a switch for the tutorial visuals.
    bool g_bilinear = true;

    // Skinning — per-frame skinning matrices S[b] = bone.localToWorld·bindpose
    // (baked at export). A skinned vertex computes posWS = Σ w_b·(S[b]·posOS).
    static const int kMaxBones = 128;
    float4x4 _BoneMatrices[kMaxBones];
    int      _BoneCount = 0;
    bool     _SKINNED   = false;

    // Weighted sum of the bone skinning matrices for one vertex (linear blend
    // skinning). Lives here so both the vertex path and the shadow caster can use
    // it. No weights -> identity (mesh-local already equals world).
    inline float4x4 BlendSkinMatrix(const int* idx, const float* w)
    {
        float4x4 S;                       // identity by default
        bool any = false;
        for (int i = 0; i < 4; ++i) {
            if (w[i] == 0.0f) continue;
            int b = idx[i];
            if (b < 0 || b >= _BoneCount) continue;
            float4x4 wb = _BoneMatrices[b] * w[i];
            S = any ? (S + wb) : wb;       // start from the first weighted term
            any = true;
        }
        return any ? S : float4x4();
    }

    // Shadow map
    static const int kShadowRes = 2048;
    float4x4 _LightVP;                // light view-projection for shadow coord transform
    float*   _ShadowDepth = nullptr;  // pointer to kShadowRes² depth buffer in [0,1]
    float    _ShadowBias  = 0.002f;   // constant depth bias to prevent self-shadowing acne
    bool     _SHADOWS_ENABLED = false;

    // Spot light shadow map (index-0 spot only).
    float4x4 _SpotLightVP;
    float*   _SpotShadowDepth      = nullptr;
    bool     _SPOT_SHADOWS_ENABLED = false;
    float    _SpotShadowBias       = 0.005f;
    Color _BaseColor;
    Color _SpecColor;
    real _Cutoff;
    Color _EmissionColor;
    float4 unity_FogColor;

    float2 _Tiling = Vec2f(1.0f, 1.0f);
    float2 _Offset = Vec2f(0, 0);

    float2 TRANSFORM_TEX(float2 texcoord, Texture2D* tex)
    {
        return float2(texcoord * _Tiling + _Offset);
    }

    half _Surface = 1;
    bool _NORMALMAP = true;
    bool BUMP_SCALE_NOT_SUPPORTED = true;
    bool UNITY_UV_STARTS_AT_TOP = false;
    bool _EMISSION = false;


    enum ZTest
    {
        Off,
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    //暂时硬编码为小的才计算, 深度缓存默认为写入
    ZTest z_test = Less;
    bool z_write = true;

    // ---------------------------------------------------------------------
    // Debug visualization (for tutorial screenshots). Default DV_NONE keeps
    // the normal render path untouched. Set g_debugView before a Draw call.
    // ---------------------------------------------------------------------
    enum DebugView {
        DV_NONE = 0,
        DV_ALBEDO,        // base color only, no lighting
        DV_NORMAL_GEOM,   // interpolated geometry normal (world space)
        DV_NORMAL_MAPPED, // per-pixel normal after normal map
        DV_UV,            // texture coordinates as red/green
        DV_WIRE,          // depth-tested wireframe
        DV_THREADS,       // tint each rasterizer thread's Y-strip
        DV_BAKEDGI,       // bakedGI / lightmap radiance as color (no direct light)
        DV_LIGHTMAPUV     // lightmapUV as red/green (diagnose lightmap sampling)
    };
    int g_debugView      = DV_NONE;
    int g_dbgRowsPerStrip = 1; // rows handled by one rasterizer thread
    int g_dbgThreadCount  = 1; // number of rasterizer threads

    // Distinct colors for the per-thread strip overlay.
    inline float4 DebugStripColor(int strip)
    {
        static const float4 pal[8] = {
            float4(0.95f, 0.30f, 0.32f, 1), float4(0.30f, 0.70f, 0.95f, 1),
            float4(0.45f, 0.85f, 0.45f, 1), float4(0.95f, 0.75f, 0.25f, 1),
            float4(0.75f, 0.45f, 0.95f, 1), float4(0.30f, 0.85f, 0.80f, 1),
            float4(0.95f, 0.55f, 0.35f, 1), float4(0.65f, 0.65f, 0.70f, 1),
        };
        return pal[((strip % 8) + 8) % 8];
    }

    inline bool DepthTest(float depth, float depthBuffer)
    {
        switch (z_test)
        {
        case Always:
            return true;
        case Equal:
            return depth == depthBuffer;
        case Greater:
            return depth > depthBuffer;
        case GreaterEqual:
            return depth >= depthBuffer;
        case Less:
            return depth < depthBuffer;
        case LessEqual:
            return depth <= depthBuffer;
        case NotEqual:
            return depth != depthBuffer;
        case Never:
            return false;
        default:
            return true;
        }
    }

    uint GetMeshRenderingLayer()
    {
        return 1;
    }

    static const half kSurfaceTypeOpaque = 0.0;
    static const half kSurfaceTypeTransparent = 1.0;

    // Returns true if the input value represents an opaque surface
    bool IsSurfaceTypeOpaque(half surfaceType)
    {
        return (surfaceType == kSurfaceTypeOpaque);
    }

    // Returns true if the input value represents a transparent surface
    bool IsSurfaceTypeTransparent(half surfaceType)
    {
        return (surfaceType == kSurfaceTypeTransparent);
    }

    bool IsOnlyAOLightingFeatureEnabled()
    {
        return false;
    }

    // URP's per-feature debug flags. Outside the lighting-debug view, every
    // feature is enabled — returning false here silently dropped GI, emission,
    // additional/vertex lights and AO from the final color. Normal render = all on.
    bool IsLightingFeatureEnabled(std::string feature)
    {
        return true;
    }
}
