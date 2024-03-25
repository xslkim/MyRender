#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "Program.hpp"
#include "BRDF.hpp"
#include "GlobalIllumination.hpp"

using namespace std;
namespace gpu
{
    float3 _Metallic(0.6f, 0.6f, 0.6);  //这两个个参数是可以在Unity 材质编辑器里面调试的
    real _Smoothness = 0.5f; 
    half4 SampleMetallicSpecGloss(float2 uv, half albedoAlpha)
    {
        half4 specGloss;
        specGloss.rgb = _Metallic.rrr;
        specGloss.a = _Smoothness;

        return specGloss;
    }

}
