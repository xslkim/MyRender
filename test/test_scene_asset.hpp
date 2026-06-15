#pragma once
#include <fstream>
#include <iostream>
#include "SceneAsset.hpp"
#include "test_utils.hpp"

// T1.1 — parse the real Unity export and assert key values survive the trip.
// Run from the repo root so the relative asset paths resolve.
inline void test_scene_asset()
{
    const std::string root = "assets/unity_export/ValidationScene/";

    // ---- scene.json ----
    std::ifstream sf(root + "scene.json");
    assert(sf.is_open() && "ValidationScene/scene.json not found — export it from Unity first");
    asset::SceneAsset scene = nlohmann::json::parse(sf).get<asset::SceneAsset>();

    assert(scene.name == "ValidationScene");
    assert(scene.objects.size() == 4);

    // Ground: scale (2,1,2) -> localToWorld diag (2,1,2); worldToLocal diag (0.5,1,0.5).
    const asset::ObjectAsset& ground = scene.objects[0];
    assert(ground.name == "Ground");
    assert(ground.materials.size() == 1);
    assert(equal(ground.matrix[0][0], 2.0f));
    assert(equal(ground.matrix[2][2], 2.0f));
    assert(equal(ground.worldToLocal[0][0], 0.5f));

    // Cube: localToWorld translation column = (-2.5, 0.5, 0).
    const asset::ObjectAsset& cube = scene.objects[1];
    assert(cube.name == "Cube");
    assert(equal(cube.matrix[0][3], -2.5f));
    assert(equal(cube.matrix[1][3],  0.5f));

    // Camera projection (verbatim OpenGL form): m00=1.07886, m11=1.73205, m32=-1.
    const asset::CameraAsset& cam = scene.camera;
    assert(equal(cam.projection[0][0], 1.07886f, 0.001f));
    assert(equal(cam.projection[1][1], 1.73205f, 0.001f));
    assert(equal(cam.projection[3][2], -1.0f));
    assert(equal(cam.worldToCamera[0][0], 1.0f));
    assert(equal(cam.near, 0.3f));

    // Main light direction.
    assert(equal(scene.light.direction.x, -0.321393818f, 0.001f));
    assert(equal(scene.light.direction.y, -0.766044557f, 0.001f));
    assert(equal(scene.light.direction.z,  0.5566703f,   0.001f));

    // ---- a material file ----
    std::ifstream mf(root + "materials/Cube_Mat.mat.json");
    assert(mf.is_open());
    asset::MaterialAsset mat = nlohmann::json::parse(mf).get<asset::MaterialAsset>();
    assert(mat.shaderModel == "Lit");
    assert(mat.cull == "back");
    assert(mat.surfaceType == "opaque");
    assert(equal(mat.baseColor.r, 0.8f, 0.001f));
    assert(equal(mat.baseColor.g, 0.2f, 0.01f));
    assert(mat.baseMap.empty());

    std::cout << "[PASS] test_scene_asset\n";
}
