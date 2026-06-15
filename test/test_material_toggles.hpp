#pragma once
#include <iostream>
#include "LitMat.hpp"
#include "ShaderGlobal.hpp"
#include "test_utils.hpp"

// T2.1 -LitMat::UpdateGpuParameter must flip the URP keyword toggles to match
// which maps are bound. Verified without file I/O by poking the map pointers.
inline void test_material_toggles()
{
    Texture2D dummy; // width=0, buffer=nullptr -fine, we only need a non-null ptr

    // No maps -> every map toggle off.
    LitMat bare;
    bare.UpdateGpuParameter();
    CHECK(gpu::_SPECGLOSSMAP == false);
    CHECK(gpu::_OCCLUSIONMAP == false);
    CHECK(gpu::_EMISSION     == false);
    CHECK(gpu::_NORMALMAP    == false);

    // All maps bound -> toggles on, pointers forwarded.
    LitMat full;
    full.normalMap            = &dummy;
    full.specularMap          = &dummy;
    full.occlusionMap         = &dummy;
    full.normalScale          = 0.5f;
    full.occlusionStrength    = 0.7f;
    full.emissionColor        = float3(1, 0, 0);
    full.smoothnessFromAlbedo = true;
    full.UpdateGpuParameter();

    CHECK(gpu::_NORMALMAP    == true && gpu::_BumpMap     == &dummy);
    CHECK(gpu::_SPECGLOSSMAP == true && gpu::_SpecGlossMap == &dummy);
    CHECK(gpu::_OCCLUSIONMAP == true && gpu::_OcclusionMap == &dummy);
    CHECK(gpu::_SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A == true);
    CHECK(gpu::_EMISSION == true);                 // from non-black emissionColor
    CHECK(equal(gpu::_BumpScale, 0.5f));
    CHECK(equal(gpu::_OcclusionStrength, 0.7f));

    std::cout << "[PASS] test_material_toggles\n";
}
