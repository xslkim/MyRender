#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "ShaderGlobal.hpp"
#include <string>

using namespace std;
namespace gpu
{
    float4 mul(float4x4 mat, float4 v)
    {
        return mat * v;
    }

    float3 mul(float3x3 mat, float3 v)
    {
        return mat * v;
    }

    float3 mul(float3 v, float3x3 mat)
    {
        return  v * mat;
    }

    float3 cross(float3 a, float3 b)
    {
        return vector_cross(a, b);
    }

    float3 normalize(float3 v)
    {
        return vector_normalize(v);
    }

    float max(float a, float b)
    {
        return Max(a, b);
    }

    float min(float a, float b)
    {
        return Min(a, b);
    }

    real3 min(real3 a, real3 b)
    {
        real3 out;
        out.x = min(a.x, b.x);
        out.y = min(a.y, b.y);
        out.z = min(a.z, b.z);
        return out;
    }

    real3 abs(real3 a)
    {
        real3 out;
        out.x = Abs(a.x);
        out.y = Abs(a.y);
        out.z = Abs(a.z);
        return out;
    }

    real abs(real a)
    {
        return Abs(a);
    }

    real3 max(real3 a, real3 b)
    {
        real3 out;
        out.x = Max(a.x, b.x);
        out.y = Max(a.y, b.y);
        out.z = Max(a.z, b.z);
        return out;
    }

    float2 rcp(float2 x) {
        return 1.0f / x;
    }

    float rsqrt(float number)
    {
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
        x2 = number * 0.5F;
        y = number;
        i = *(long*)&y;                       
        i = 0x5f3759df - (i >> 1);              
        y = *(float*)&i;
        y = y * (threehalfs - (x2 * y * y));   // 1st iteration 
    //      y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
        return y;
    }

    float saturate(float a)
    {
        if (a < 0) return 0;
        if (a > 1) return 1;
        return a;
    }

    float3 saturate(float3 a)
    {
        float3 out;
        out.x = saturate(a.x);
        out.y = saturate(a.y);
        out.z = saturate(a.z);
        return out;
    }

    real Sq(real a)
    {
        return a * a;
    }

    float4x4 GetObjectToWorldMatrix()
    {
        return UNITY_MATRIX_M;
    }

    float4x4 GetWorldToObjectMatrix()
    {
        return UNITY_MATRIX_I_M;
    }

    float4x4 GetPrevObjectToWorldMatrix()
    {
        return UNITY_PREV_MATRIX_M;
    }

    float4x4 GetPrevWorldToObjectMatrix()
    {
        return UNITY_PREV_MATRIX_I_M;
    }

    float4x4 GetWorldToViewMatrix()
    {
        return UNITY_MATRIX_V;
    }

    float4x4 GetWorldToHClipMatrix()
    {
        return UNITY_MATRIX_VP;
    }

    float4x4 GetViewToHClipMatrix()
    {
        return UNITY_MATRIX_P;
    }

    real3 SafeNormalize(float3 inVec)
    {
        float dp3 = max(FLT_MIN, dot(inVec, inVec));
        return inVec * rsqrt(dp3);
    }

    float3 TransformObjectToWorld(float3 positionOS)
    {
        return mul(GetObjectToWorldMatrix(), float4(positionOS, 1.0)).xyz;
    }

    float3 TransformWorldToView(float3 positionWS)
    {
        return mul(GetWorldToViewMatrix(), float4(positionWS, 1.0)).xyz;
    }

    float4 TransformWorldToHClip(float3 positionWS)
    {
        return mul(GetWorldToHClipMatrix(), float4(positionWS, 1.0));
    }


    VertexPositionInputs GetVertexPositionInputs(float3 positionOS)
    {
        VertexPositionInputs input;
        input.positionWS = TransformObjectToWorld(positionOS);
        input.positionVS = TransformWorldToView(input.positionWS);
        input.positionCS = TransformWorldToHClip(input.positionWS);

        //https://zhuanlan.zhihu.com/p/656243278
        //https://forum.unity.com/threads/confused-on-ndc-space.1024414/
        float4 ndc = input.positionCS * 0.5f;
        input.positionNDC.xy = float2(ndc.x, ndc.y * _ProjectionParams.x) + ndc.w;
        input.positionNDC.zw = input.positionCS.zw;

        return input;
    }

    // Transforms normal from object to world space 
    //https://zhuanlan.zhihu.com/p/261667233
    float3 TransformObjectToWorldNormal(float3 normalOS, bool doNormalize = true)
    {
        //反过来乘， 就是乘矩阵的转置矩阵。 法线矩阵就是 obj2world 的逆矩阵的转置,
        float3 normalWS = mul(normalOS, GetWorldToObjectMatrix().GetMinor(3, 3));

        float3x3 world2obj = GetWorldToObjectMatrix().GetMinor(3, 3);
        float3x3 world2obj_t = world2obj.Transpose();
        //对比一下看是否一样
        float3 normalWS1 = mul(world2obj_t, normalOS);

        if (doNormalize)
            return SafeNormalize(normalWS);
        return normalWS;
    }

    //https://zhuanlan.zhihu.com/p/103546030
    real GetOddNegativeScale()
    {
        //real4 unity_WorldTransformParams;
        //return unity_WorldTransformParams.w >= 0.0 ? 1.0 : -1.0;
        //暂时不支持缩放的负数，直接返回1
        return 1;
    }

    float3 TransformObjectToWorldDir(float3 dirOS, bool doNormalize = true)
    {
        float3 dirWS = mul(GetObjectToWorldMatrix().GetMinor(3, 3), dirOS);
        if (doNormalize)
            return SafeNormalize(dirWS);

        return dirWS;
    }

    VertexNormalInputs GetVertexNormalInputs(float3 normalOS, float4 tangentOS)
    {
        VertexNormalInputs tbn;

        // mikkts space compliant. only normalize when extracting normal at frag.
        real sign = real(tangentOS.w) * GetOddNegativeScale();
        tbn.normalWS = TransformObjectToWorldNormal(normalOS);
        tbn.tangentWS = real3(TransformObjectToWorldDir(tangentOS.xyz));
        tbn.bitangentWS = real3(cross(tbn.normalWS, float3(tbn.tangentWS))) * sign;
        return tbn;
    }

    float3 GetCameraPositionWS()
    {
        return _WorldSpaceCameraPos;
    }

    float3 GetCurrentViewPosition()
    {
        return GetCameraPositionWS();
    }

    half3 GetWorldSpaceNormalizeViewDir(float3 positionWS)
    {
        // Perspective
        float3 V = GetCurrentViewPosition() - positionWS;
        return half3(normalize(V));
    }

    float3 GetWorldSpaceViewDir(float3 positionWS)
    {
        return GetCurrentViewPosition() - positionWS;
    }

    real ComputeFogFactorZ0ToFar(float z)
    {
        //float fogFactor = saturate(z * unity_FogParams.z + unity_FogParams.w);
        //return real(fogFactor);
        return 0;
    }

#define UNITY_Z_0_FAR_FROM_CLIPSPACE(coord) max((coord - _ProjectionParams.y)/(-_ProjectionParams.z-_ProjectionParams.y)*_ProjectionParams.z, 0)
    real ComputeFogFactor(float zPositionCS)
    {
        float clipZ_0Far = UNITY_Z_0_FAR_FROM_CLIPSPACE(zPositionCS);
        return ComputeFogFactorZ0ToFar(clipZ_0Far);
    }


    half4 SAMPLE_TEXTURE2D(const Texture2D* tex, const SamplerState& state, float2 uv)
    {
        if (tex == nullptr)
        {
            return half(1);
        }
        return tex->SamplerPoint(uv.x, uv.y);
    }


    half4 SampleAlbedoAlpha(float2 uv, Texture2D* albedoAlphaMap, SamplerState sampler_albedoAlphaMap)
    {
        return half4(SAMPLE_TEXTURE2D(albedoAlphaMap, sampler_albedoAlphaMap, uv));
    }

    void AlphaDiscard(real alpha, real cutoff, real offset = real(0.0))
    {
#ifdef _ALPHATEST_ON
        if (IsAlphaDiscardEnabled())
            clip(alpha - cutoff + offset);
#endif
    }

    half4 SampleSpecularSmoothness(float2 uv, half alpha, half4 specColor, Texture2D* specMap, SamplerState sampler_specMap)
    {
        half4 specularSmoothness = half4(0, 0, 0, 1);
        if (_SPECGLOSSMAP)
        {
            specularSmoothness = SAMPLE_TEXTURE2D(specMap, sampler_specMap, uv) * specColor;
        }
        else if (_SPECULAR_COLOR)
        {
            specularSmoothness = specColor;
        }

        if (_GLOSSINESS_FROM_BASE_ALPHA)
        {
            specularSmoothness.a = alpha;
        }
        return specularSmoothness;
    }

    real3 UnpackNormal(real4 packedNormal, real scale = 1.0)
    {
        real3 normal;
        normal.xyz = packedNormal.xyz * 2.0f - 1.0f;
        normal.xy *= scale;
        return normal;
    }

    half3 SampleNormal(float2 uv, Texture2D* bumpMap, SamplerState sampler_bumpMap, half scale = half(1.0))
    {
        if (_NORMALMAP && bumpMap != nullptr)
        {
            half4 n = SAMPLE_TEXTURE2D(bumpMap, sampler_bumpMap, uv);
            return UnpackNormal(n);
        }
        return half3(0.0, 0.0, 1.0);
    }

    
    half3 SampleEmission(float2 uv, half3 emissionColor, Texture2D* emissionMap, SamplerState sampler_emissionMap)
    {
        if (!_EMISSION)
            return half3(0, 0, 0);
        else
            return SAMPLE_TEXTURE2D(emissionMap, sampler_emissionMap, uv).rgb * emissionColor;
    }

    real3 TransformTangentToWorld(float3 normalTS, half3x3 tangentToWorld, bool doNormalize = false)
    {
        real3 result = mul(normalTS, tangentToWorld);
        if (doNormalize)
            return SafeNormalize(result);
        return result;
    }

    float3 NormalizeNormalPerPixel(float3 normalWS)
    {
        if (_NORMALMAP)
            return SafeNormalize(normalWS);
        else
            return normalize(normalWS);
    }

    float4 GetScaledScreenParams()
    {
        return _ScaledScreenParams;
    }

    void TransformScreenUV(float2& uv, float screenHeight)
    {
        if (UNITY_UV_STARTS_AT_TOP)
            uv.y = screenHeight - (uv.y * _ScaleBiasRt.x + _ScaleBiasRt.y * screenHeight);
    }

    void TransformScreenUV(float2& uv)
    {
        if (UNITY_UV_STARTS_AT_TOP)
            TransformScreenUV(uv, GetScaledScreenParams().y);
    }

    void TransformNormalizedScreenUV(float2& uv)
    {
        if (UNITY_UV_STARTS_AT_TOP)
            TransformScreenUV(uv, 1.0);
    }


    float2 GetNormalizedScreenSpaceUV(float2 positionCS)
    {
        float2 normalizedScreenSpaceUV = positionCS.xy * rcp(GetScaledScreenParams().xy);
        TransformNormalizedScreenUV(normalizedScreenSpaceUV);
        return normalizedScreenSpaceUV;
    }

    float2 GetNormalizedScreenSpaceUV(float4 positionCS)
    {
        return GetNormalizedScreenSpaceUV(positionCS.xy);
    }

    uint GetMeshRenderingLightLayer() 
    {
        return 1; 
    }

    half4 CalculateShadowMask(InputData inputData)
    {
        return half4(1, 1, 1, 1);;
    }

    struct AmbientOcclusionFactor
    {
        half indirectAmbientOcclusion;
        half directAmbientOcclusion;
    };

    AmbientOcclusionFactor GetScreenSpaceAmbientOcclusion(float2 normalizedScreenSpaceUV)
    {
        AmbientOcclusionFactor aoFactor;
        aoFactor.directAmbientOcclusion = 1;
        aoFactor.indirectAmbientOcclusion = 1;
        return aoFactor;
    }

    AmbientOcclusionFactor CreateAmbientOcclusionFactor(float2 normalizedScreenSpaceUV, half occlusion)
    {
        AmbientOcclusionFactor aoFactor = GetScreenSpaceAmbientOcclusion(normalizedScreenSpaceUV);

        aoFactor.indirectAmbientOcclusion = min(aoFactor.indirectAmbientOcclusion, occlusion);
        return aoFactor;
    }

    AmbientOcclusionFactor CreateAmbientOcclusionFactor(InputData inputData, SurfaceData surfaceData)
    {
        return CreateAmbientOcclusionFactor(inputData.normalizedScreenSpaceUV, surfaceData.occlusion);
    }

#define USE_CLUSTERED_LIGHTING 1
    Light GetMainLight()
    {
        Light light;
        light.direction = half3(_MainLightPosition.xyz);
#if USE_CLUSTERED_LIGHTING
        light.distanceAttenuation = 1.0;
#else
        light.distanceAttenuation = unity_LightData.z; // unity_LightData.z is 1 when not culled by the culling mask, otherwise 0.
#endif
        light.shadowAttenuation = 1.0;
        light.color = _MainLightColor.rgb;

        light.layerMask = _MainLightLayerMask;

        return light;
    }

    half MainLightRealtimeShadow(float4 shadowCoord)
    {
#if !defined(MAIN_LIGHT_CALCULATE_SHADOWS)
        return half(1.0);
#elif defined(_MAIN_LIGHT_SHADOWS_SCREEN) && !defined(_SURFACE_TYPE_TRANSPARENT)
        return SampleScreenSpaceShadowmap(shadowCoord);
#else
        ShadowSamplingData shadowSamplingData = GetMainLightShadowSamplingData();
        half4 shadowParams = GetMainLightShadowParams();
        return SampleShadowmap(TEXTURE2D_ARGS(_MainLightShadowmapTexture, sampler_MainLightShadowmapTexture), shadowCoord, shadowSamplingData, shadowParams, false);
#endif
    }

    half MixRealtimeAndBakedShadows(half realtimeShadow, half bakedShadow, half shadowFade)
    {
#if defined(LIGHTMAP_SHADOW_MIXING)
        return min(lerp(realtimeShadow, 1, shadowFade), bakedShadow);
#else
        return lerp(realtimeShadow, bakedShadow, shadowFade);
#endif
    }

    half MainLightShadow(float4 shadowCoord, float3 positionWS, half4 shadowMask, half4 occlusionProbeChannels)
    {
        half realtimeShadow = MainLightRealtimeShadow(shadowCoord);

#ifdef CALCULATE_BAKED_SHADOWS
        half bakedShadow = BakedShadow(shadowMask, occlusionProbeChannels);
#else
        half bakedShadow = half(1.0);
#endif

#ifdef MAIN_LIGHT_CALCULATE_SHADOWS
        half shadowFade = GetMainLightShadowFade(positionWS);
#else
        half shadowFade = half(1.0);
#endif

        return MixRealtimeAndBakedShadows(realtimeShadow, bakedShadow, shadowFade);
    }

    Light GetMainLight(float4 shadowCoord, float3 positionWS, half4 shadowMask)
    {
        Light light = GetMainLight();
        light.shadowAttenuation = MainLightShadow(shadowCoord, positionWS, shadowMask, _MainLightOcclusionProbes);

#if defined(_LIGHT_COOKIES)
        real3 cookieColor = SampleMainLightCookie(positionWS);
        light.color *= cookieColor;
#endif

        return light;
    }

    Light GetMainLight(InputData inputData, half4 shadowMask, AmbientOcclusionFactor aoFactor)
    {
        Light light = GetMainLight(inputData.shadowCoord, inputData.positionWS, shadowMask);

#if defined(_SCREEN_SPACE_OCCLUSION) && !defined(_SURFACE_TYPE_TRANSPARENT)
        if (IsLightingFeatureEnabled(DEBUGLIGHTINGFEATUREFLAGS_AMBIENT_OCCLUSION))
        {
            light.color *= aoFactor.directAmbientOcclusion;
        }
#endif

        return light;
    }


    bool IsMatchingLightLayer(uint lightLayers, uint renderingLayers)
    {
        return (lightLayers & renderingLayers) != 0;
    }


    float3 MixFogColor(float3 fragColor, float3 fogColor, float fogFactor)
    {
#if defined(FOG_LINEAR) || defined(FOG_EXP) || defined(FOG_EXP2)
        if (IsFogEnabled())
        {
            float fogIntensity = ComputeFogIntensity(fogFactor);
            fragColor = lerp(fogColor, fragColor, fogIntensity);
        }
#endif
        return fragColor;
    }


    float3 MixFog(float3 fragColor, float fogFactor)
    {
        return MixFogColor(fragColor, unity_FogColor.rgb, fogFactor);
    }

    half OutputAlpha(half outputAlpha, half surfaceType = half(0.0))
    {
        return surfaceType == 1 ? outputAlpha : half(1.0);
    }

    float3 reflect(float3 i, float3 n) {
        return i - 2.0f * n * dot(i, n);
    }

    float Pow4(float baseValue) {
        return pow(baseValue, 4.0);
    }

    float Pow(float base, float power) {
        return pow(base, power);
    }

    float3 Pow(float3 base, float3 power) {
        float3 out;
        out.x = pow(base.x, power.x);
        out.y = pow(base.y, power.y);
        out.z = pow(base.z, power.z);
        return out;
    }

    


    real InitializeInputDataFog(float4 positionWS, real vertFogFactor)
    {
        real fogFactor = 0.0;
        // Compiler eliminates unused math --> matrix.column_z * vec
        float viewZ = -(mul(UNITY_MATRIX_V, positionWS).z);
        // View Z is 0 at camera pos, remap 0 to near plane.
        float nearToFarZ = max(viewZ - _ProjectionParams.y, 0);
        fogFactor = ComputeFogFactorZ0ToFar(nearToFarZ);
        return fogFactor;
    }
}


