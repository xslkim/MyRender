#pragma once
#include "ShaderFunction.hpp"
#include "LitInput.hpp"


namespace gpu
{
    half Alpha(half albedoAlpha, half4 color, half cutoff)
    {
        half alpha = albedoAlpha * color.a;
        //alpha = AlphaDiscard(alpha, cutoff);
        return alpha;
    }
    bool _ALPHAMODULATE_ON = false;
    half3 AlphaModulate(half3 albedo, half alpha)
    {
        if(_ALPHAMODULATE_ON)
            return lerp(half3(1.0, 1.0, 1.0), albedo, alpha);
        else
            return albedo;
    }

    half SampleOcclusion(float2 uv)
    {
        if (!_OCCLUSIONMAP || _OcclusionMap == nullptr)
            return half(1.0);
        // URP reads occlusion from the green channel of the mask/occlusion map.
        half occ = SAMPLE_TEXTURE2D(_OcclusionMap, sampler_OcclusionMap, uv).g;
        return LerpWhiteTo(occ, _OcclusionStrength);
    }

    inline void InitializeStandardLitSurfaceData(float2 uv, SurfaceData& outSurfaceData)
    {
        half4 albedoAlpha = SampleAlbedoAlpha(uv, _BaseMap, sampler_BaseMap);
        outSurfaceData.alpha = Alpha(albedoAlpha.a, _BaseColor, _Cutoff);

        half4 specGloss = SampleMetallicSpecGloss(uv, albedoAlpha.a);
        outSurfaceData.albedo = albedoAlpha.rgb * _BaseColor.rgb;
        outSurfaceData.albedo = AlphaModulate(outSurfaceData.albedo, outSurfaceData.alpha);

        outSurfaceData.metallic = specGloss.r;
        outSurfaceData.specular = half3(0.0, 0.0, 0.0);

        outSurfaceData.smoothness = specGloss.a;
        outSurfaceData.normalTS = SampleNormal(uv, _BumpMap, sampler_BumpMap, _BumpScale);
        outSurfaceData.occlusion = SampleOcclusion(uv);
        outSurfaceData.emission = SampleEmission(uv, _EmissionColor.rgb, _EmissionMap, sampler_EmissionMap);

        outSurfaceData.clearCoatMask = half(0.0);
        outSurfaceData.clearCoatSmoothness = half(0.0);
    }

    void LitInitializeInputData(Varyings input, half3 normalTS, InputData& inputData)
    {
        inputData = (InputData)0;

        inputData.positionWS = input.positionWS;
        inputData.positionCS = input.positionCS;

        half3 viewDirWS = GetWorldSpaceNormalizeViewDir(input.positionWS);
        float sgn = input.tangentWS.w;      // should be either +1 or -1
        float3 bitangent = sgn * cross(input.normalWS.xyz, input.tangentWS.xyz);
        half3x3 tangentToWorld = half3x3(input.tangentWS.xyz, bitangent.xyz, input.normalWS.xyz);
        inputData.tangentToWorld = tangentToWorld;
        inputData.normalWS = TransformTangentToWorld(normalTS, tangentToWorld);
        inputData.normalWS = NormalizeNormalPerPixel(inputData.normalWS);
        inputData.viewDirectionWS = viewDirWS;
        //暂时不支持，烘焙环境光
        //inputData.bakedGI = SAMPLE_GI(input.staticLightmapUV, input.vertexSH, inputData.normalWS);
        inputData.normalizedScreenSpaceUV = GetNormalizedScreenSpaceUV(input.positionCS);

        // Shadow coordinate: project world position into light clip space per-pixel.
        inputData.shadowCoord = _SHADOWS_ENABLED
            ? _LightVP * float4(inputData.positionWS.x, inputData.positionWS.y, inputData.positionWS.z, 1.0f)
            : float4(0.0f, 0.0f, 0.0f, 0.0f);
    }


    void LitVertexShader(const Attributes* attributes, Varyings* varyings)
    {
        const Attributes& input = *attributes;
        Varyings& output = *varyings;


        VertexPositionInputs vertexInput = GetVertexPositionInputs(input.positionOS.xyz);
        VertexNormalInputs normalInput = GetVertexNormalInputs(input.normalOS, input.tangentOS);

        //暂时不考虑，附加光源
        //half3 vertexLight = VertexLighting(vertexInput.positionWS, normalInput.normalWS);

        //暂时不考虑，雾效果
        half fogFactor = 0;
        output.uv = TRANSFORM_TEX(input.texcoord, _BaseMap);

        // already normalized from normal transform to WS.
        output.normalWS.xyz = normalInput.normalWS;

        //REQUIRES_WORLD_SPACE_TANGENT_INTERPOLATOR  这个宏是生效的
        real sign = input.tangentOS.w * GetOddNegativeScale();
        half4 tangentWS = half4(normalInput.tangentWS.xyz, sign);
        output.tangentWS = tangentWS;

        //暂不考虑，烘焙光照贴图
        //OUTPUT_LIGHTMAP_UV(input.staticLightmapUV, unity_LightmapST, output.staticLightmapUV);
        output.positionWS = vertexInput.positionWS;

        output.fogFactor = fogFactor;

        output.positionCS = vertexInput.positionCS;
    }

    half4 LitFragmentShader(Varyings& input)
    {
        SurfaceData surfaceData;
        InitializeStandardLitSurfaceData(input.uv, surfaceData);

        InputData inputData;
        LitInitializeInputData(input, surfaceData.normalTS, inputData);

        if (g_debugView != DV_NONE) {
            if (g_debugView == DV_ALBEDO) return half4(surfaceData.albedo, 1);
            if (g_debugView == DV_NORMAL_GEOM) {
                half3 n = input.normalWS.xyz;
                return half4(n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f, n.z * 0.5f + 0.5f, 1);
            }
            if (g_debugView == DV_NORMAL_MAPPED) {
                half3 n = inputData.normalWS;
                return half4(n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f, n.z * 0.5f + 0.5f, 1);
            }
            if (g_debugView == DV_UV)
                return half4(input.uv.x - floorf(input.uv.x), input.uv.y - floorf(input.uv.y), 0, 1);
        }

        half4 color = UniversalFragmentPBR(inputData, surfaceData);
        color.rgb = MixFog(color.rgb, inputData.fogCoord);
        color.a = OutputAlpha(color.a, IsSurfaceTypeTransparent(_Surface));

        return color;
    }

}
