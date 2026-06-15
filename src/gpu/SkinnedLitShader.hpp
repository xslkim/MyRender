#pragma once
#include "ShaderFunction.hpp"
#include "LitInput.hpp"
#include "LitShader.hpp"

// ---------------------------------------------------------------------------
// Skinned Lit vertex stage — linear blend skinning (design §9.2).
//
// The bake produced, per bone, S[b] = bone.localToWorld · bindpose[b], which maps
// a vertex from mesh-local straight to world space. So a skinned vertex needs NO
// separate object matrix M:
//     posWS  = Σ_b w_b · (S[b] · posOS)
//     normWS = normalize( (Σ_b w_b · S[b])_3x3 · normOS )
// The fragment stage is unchanged — we reuse LitFragmentShader.
// ---------------------------------------------------------------------------
namespace gpu
{
    inline void LitSkinnedVertexShader(const Attributes* attributes, Varyings* varyings)
    {
        const Attributes& input = *attributes;
        Varyings& output = *varyings;

        float4x4 S  = BlendSkinMatrix(input.boneIndex, input.boneWeight);
        float3x3 S3 = S.GetMinor(3, 3);

        // Skin straight to world space (no UNITY_MATRIX_M for skinned meshes).
        float3 positionWS = mul(S, input.positionOS).xyz;
        output.positionWS = positionWS;
        output.positionCS = mul(UNITY_MATRIX_VP, float4(positionWS, 1.0f));

        output.normalWS.xyz = SafeNormalize(mul(S3, input.normalOS));

        real sign = real(input.tangentOS.w) * GetOddNegativeScale();
        float3 tangentWS = SafeNormalize(mul(S3, float3(input.tangentOS.xyz)));
        output.tangentWS = half4(tangentWS, sign);

        output.uv        = TRANSFORM_TEX(input.texcoord, _BaseMap);
        output.fogFactor = 0;
    }
}
