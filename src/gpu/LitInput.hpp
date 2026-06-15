#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "Program.hpp"
#include "BRDF.hpp"
#include "GlobalIllumination.hpp"


namespace gpu
{
    float3 _Metallic(0.6f, 0.6f, 0.6);  //这两个个参数是可以在Unity 材质编辑器里面调试的
    real _Smoothness = 0.5f;

    // URP metallic workflow: when a mask map is bound, metallic = map.r and
    // smoothness = map.a * _Smoothness; otherwise the scalar fallbacks are used.
    // _SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A pulls smoothness from albedo.a instead.
    half4 SampleMetallicSpecGloss(float2 uv, half albedoAlpha)
    {
        half4 specGloss;

        if (_SPECGLOSSMAP)
        {
            specGloss = SAMPLE_TEXTURE2D(_SpecGlossMap, sampler_SpecGlossMap, uv);
            if (_SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A)
                specGloss.a = albedoAlpha * _Smoothness;
            else
                specGloss.a = specGloss.a * _Smoothness;
        }
        else
        {
            specGloss.rgb = _Metallic.rrr;
            specGloss.a   = _SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A ? (albedoAlpha * _Smoothness) : _Smoothness;
        }

        return specGloss;
    }

}
