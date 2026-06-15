#pragma once
#include <iostream>
#include "Config.hpp"
#include "Render.hpp"
#include "UnitySceneLoader.hpp"
#include "ShaderGlobal.hpp"
#include "test_utils.hpp"

// T1.4 — the golden gate. Load the Unity scene, upload its matrices through the
// real Render API, then push a known cube vertex through the same matrix path
// the vertex shader uses (posWS = M·posOS; posCS = VP·posWS). The reference was
// computed independently (numpy) from the exported matrices, so this catches
// row-major/column-major, VP=P·V order, and upload bugs.
inline void test_vertex_transform()
{
    Config::scene_path = "assets/unity_export/ValidationScene/";
    SceneModel model = UnitySceneLoader::Load("scene.json");

    const RenderObject& cube = model.objects[1]; // Cube at (-2.5, 0.5, 0)
    Render::Get().SetCamera(model.camera);
    Render::Get().SetModelMatrices(cube.localToWorld, cube.worldToLocal);

    float4 posOS(0.5f, -0.5f, 0.5f, 1.0f);            // cube triangle-0 vertex 0
    float4 posWS = gpu::UNITY_MATRIX_M  * posOS;       // M · v
    float4 posCS = gpu::UNITY_MATRIX_VP * posWS;       // (P·V) · v

    // posWS = (-2, 0, 0.5)
    assert(equal(posWS.x, -2.0f));
    assert(equal(posWS.y,  0.0f));
    assert(equal(posWS.z,  0.5f));

    // posCS = (-2.15772, -0.52430, 7.47674, 8.07207)
    assert(equal(posCS.x, -2.15772438f, 0.001f));
    assert(equal(posCS.y, -0.52429573f, 0.001f));
    assert(equal(posCS.z,  7.47673746f, 0.001f));
    assert(equal(posCS.w,  8.07207344f, 0.001f));

    // NDC = (-0.26731, -0.06495, 0.92625)
    float3 ndc = posCS.xyz / posCS.w;
    assert(equal(ndc.x, -0.26730733f, 0.001f));
    assert(equal(ndc.y, -0.06495180f, 0.001f));
    assert(equal(ndc.z,  0.92624745f, 0.001f));

    std::cout << "[PASS] test_vertex_transform\n";
}
