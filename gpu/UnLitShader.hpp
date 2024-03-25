#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "Program.hpp"
#include "ShaderFunction.hpp"
#include "Lighting.hpp"
#include <string>

using namespace std;
namespace gpu
{

    void UnLitVertexShader(const Attributes* attributes, Varyings* varyings)
    {
        const Attributes& input = *attributes;
        Varyings& output = *varyings;

        VertexPositionInputs vertexInput = GetVertexPositionInputs(input.positionOS.xyz);

        output.uv = TRANSFORM_TEX(input.texcoord, _BaseMap);
        output.positionWS.xyz = vertexInput.positionWS;
        output.positionOS = attributes->positionOS.xyz;
        output.positionCS = vertexInput.positionCS;

    }

    half4 UnLitFragmentShader(Varyings& varyings)
    {
        Varyings& input = varyings;
        half4 albedoAlpha = SampleAlbedoAlpha(input.uv, _BaseMap, sampler_BaseMap);
        return albedoAlpha;
    }
}
