#pragma once
#include "../src/core/SceneAsset.hpp"
#include "../src/core/SceneModel.hpp"
#include "../src/core/UnitySceneLoader.hpp"
#include "../src/core/ShadowPass.hpp"
#include "../src/core/Render.hpp"
#include "../src/core/Config.hpp"
#include "../src/gpu/ShaderGlobal.hpp"
#include "test_utils.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

// T3.2 -AABB corners projected by lightVP should sit near the shadow-map border.
// We load the ValidationScene (which has known geometry), compute lightVP, and
// verify that the AABB extents project close to NDC +/-1.
inline void test_shadow_aabb_fit()
{
    Config::scene_path = "assets/unity_export/ValidationScene/";
    SceneModel model = UnitySceneLoader::Load("scene.json");
    CHECK(!model.objects.empty() && "ValidationScene must have objects");

    float4x4 lightVP = ShadowPass::ComputeLightVP(model);

    auto [worldMin, worldMax] = ShadowPass::ComputeSceneAABB(model);

    float3 corners[8] = {
        { worldMin.x, worldMin.y, worldMin.z },
        { worldMax.x, worldMin.y, worldMin.z },
        { worldMin.x, worldMax.y, worldMin.z },
        { worldMax.x, worldMax.y, worldMin.z },
        { worldMin.x, worldMin.y, worldMax.z },
        { worldMax.x, worldMin.y, worldMax.z },
        { worldMin.x, worldMax.y, worldMax.z },
        { worldMax.x, worldMax.y, worldMax.z },
    };

    float maxAbsX = 0, maxAbsY = 0;
    float minDepth = 1.0f, maxDepth = 0.0f;
    for (const auto& c : corners) {
        float4 clip = lightVP * float4(c.x, c.y, c.z, 1.0f);
        float ndcX  = clip.x / clip.w;
        float ndcY  = clip.y / clip.w;
        float ndcZ  = clip.z / clip.w;
        maxAbsX = std::max(maxAbsX, std::abs(ndcX));
        maxAbsY = std::max(maxAbsY, std::abs(ndcY));
        float d = ndcZ * 0.5f + 0.5f;
        minDepth = std::min(minDepth, d);
        maxDepth = std::max(maxDepth, d);
    }

    // AABB corners should reach close to the shadow map edges.
    CHECK(maxAbsX > 0.7f && "lightVP X extent too loose -ortho fit is broken");
    CHECK(maxAbsY > 0.7f && "lightVP Y extent too loose -ortho fit is broken");
    // Depth should span most of [0,1].
    CHECK(minDepth < 0.2f && "near depth not close enough to 0");
    CHECK(maxDepth > 0.8f && "far depth not close enough to 1");

    std::cout << "[PASS] test_shadow_aabb_fit: maxAbsX=" << maxAbsX
              << " maxAbsY=" << maxAbsY
              << " depth=[" << minDepth << ", " << maxDepth << "]\n";
}

// T3.1 -After a depth-only pass the shadow buffer must have some pixels written
// (depth < 1) and they should form a gradient (center < max, objects are closer
// to the light than infinity).
inline void test_shadow_depth_pass()
{
    Render::Get().Init();
    Render::Get().SetFrontFaceSign(-1.0f);
    Config::scene_path = "assets/unity_export/ValidationScene/";
    SceneModel model = UnitySceneLoader::Load("scene.json");
    CHECK(!model.objects.empty());

    ShadowPass::Render(model);

    const float* depth = gpu::_ShadowDepth;
    CHECK(depth != nullptr && "shadow depth buffer must be allocated after Render");

    int total = gpu::kShadowRes * gpu::kShadowRes;
    float minD = 1.0f, maxD = 0.0f;
    int writtenPixels = 0;
    for (int i = 0; i < total; ++i) {
        if (depth[i] < 1.0f) {
            ++writtenPixels;
            minD = std::min(minD, depth[i]);
            maxD = std::max(maxD, depth[i]);
        }
    }

    CHECK(writtenPixels > 1000 && "shadow pass wrote too few pixels -nothing was rendered");
    CHECK(minD < maxD && "depth buffer has no gradient -all values identical");

    std::cout << "[PASS] test_shadow_depth_pass: written=" << writtenPixels
              << " depth=[" << minD << ", " << maxD << "]\n";
}
