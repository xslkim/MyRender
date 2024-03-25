#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "Program.hpp"
#include <string>

using namespace std;
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

    //这个实际上是主光源的方向
    float4 _MainLightPosition;

    half4 _MainLightColor(1,1,1,1);
    half4 _MainLightOcclusionProbes;
    uint _MainLightLayerMask = 1;
    half4 unity_LightData(0,0,0,0);
    half4 unity_LightIndices[2]; //(光源数据相关, 光源总数、光源偏移量、光源索引)

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

    bool IsLightingFeatureEnabled(string feature)
    {
        return false;
    }
}
