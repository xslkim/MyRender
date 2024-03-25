#pragma once
#include "ShaderFunction.hpp"
#include "Lighting.hpp"

using namespace std;
namespace gpu
{

    void InitializeSimpleLitSurfaceData(float2 uv, SurfaceData& outSurfaceData)
    {
        //outSurfaceData = (SurfaceData)0;

        half4 albedoAlpha = SampleAlbedoAlpha(uv, _BaseMap, sampler_BaseMap);
        outSurfaceData.alpha = albedoAlpha.a * _BaseColor.a;
        AlphaDiscard(outSurfaceData.alpha, _Cutoff);

        outSurfaceData.albedo = albedoAlpha.rgb * _BaseColor.rgb;
        if (_ALPHAPREMULTIPLY_ON)
        {
            outSurfaceData.albedo *= outSurfaceData.alpha;
        }

        half4 specularSmoothness = SampleSpecularSmoothness(uv, outSurfaceData.alpha, _SpecColor, _SpecGlossMap, sampler_SpecGlossMap);
        outSurfaceData.metallic = 0.0; // unused
        outSurfaceData.specular = specularSmoothness.rgb;
        outSurfaceData.smoothness = specularSmoothness.a;
        outSurfaceData.normalTS = SampleNormal(uv, _BumpMap, sampler_BumpMap);
        outSurfaceData.occlusion = 1.0;
        outSurfaceData.emission = SampleEmission(uv, _EmissionColor.rgb, _EmissionMap, sampler_EmissionMap);
    }



    void SimpleLitVertexShader(const Attributes* attributes, Varyings* varyings)
    {
        const Attributes& input = *attributes;
        Varyings& output = *varyings;


        VertexPositionInputs vertexInput = GetVertexPositionInputs(input.positionOS.xyz);
        VertexNormalInputs normalInput = GetVertexNormalInputs(input.normalOS, input.tangentOS);

        //half fogFactor = ComputeFogFactor(vertexInput.positionCS.z);

        output.uv = TRANSFORM_TEX(input.texcoord, _BaseMap);

        output.positionWS.xyz = vertexInput.positionWS;
        output.positionOS = attributes->positionOS.xyz;
        output.positionCS = vertexInput.positionCS;


        if (_NORMALMAP)
        {
            half3 viewDirWS = GetWorldSpaceViewDir(vertexInput.positionWS);
            output.normalWS = half4(normalInput.normalWS, viewDirWS.x);
            output.tangentWS = half4(normalInput.tangentWS, viewDirWS.y);
            output.bitangentWS = half4(normalInput.bitangentWS, viewDirWS.z);
        }
        else
        {
            output.normalWS.xyz = normalize(normalInput.normalWS);
        }

        half fogFactor = ComputeFogFactor(vertexInput.positionCS.z);
        output.fogFactor = fogFactor;
    }

    void SimpleLitInitializeInputData(Varyings& input, half3 normalTS, InputData& inputData)
    {
        //inputData = (InputData)0;
        half3 viewDirWS;
        inputData.positionWS = input.positionWS;
        if (_NORMALMAP)
        {
            //float sgn = input.tangentWS.w;      // should be either +1 or -1
            //float3 bitangent = sgn * cross(input.normalWS.xyz, input.tangentWS.xyz);
            //half3x3 tangentToWorld = half3x3(input.tangentWS.xyz, bitangent.xyz, input.normalWS.xyz);
            //inputData.tangentToWorld = tangentToWorld;
            //inputData.normalWS = TransformTangentToWorld(normalTS, tangentToWorld);
            viewDirWS = half3(input.normalWS.w, input.tangentWS.w, input.bitangentWS.w);
            inputData.tangentToWorld = half3x3(input.tangentWS.xyz, input.bitangentWS.xyz, input.normalWS.xyz);
            inputData.normalWS = TransformTangentToWorld(normalTS, inputData.tangentToWorld);
        }
        else
        {
            viewDirWS = GetWorldSpaceNormalizeViewDir(inputData.positionWS);
            inputData.normalWS = input.normalWS.xyz;
        }

        inputData.normalWS = NormalizeNormalPerPixel(inputData.normalWS);
        viewDirWS = SafeNormalize(viewDirWS);

        inputData.viewDirectionWS = viewDirWS;

        inputData.normalizedScreenSpaceUV = GetNormalizedScreenSpaceUV(input.positionCS);
    }

    half4 SimpleLitFragmentShader(Varyings& varyings)
    {
        Varyings& input = varyings;

        SurfaceData surfaceData;
        InitializeSimpleLitSurfaceData(input.uv, surfaceData);

        InputData inputData;
        SimpleLitInitializeInputData(input, surfaceData.normalTS, inputData);


        half4 color = UniversalFragmentBlinnPhong(inputData, surfaceData);
        color.rgb = MixFog(color.rgb, inputData.fogCoord);
        color.a = OutputAlpha(color.a, _Surface);
        return color;
    }

}
