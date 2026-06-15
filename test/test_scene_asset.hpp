#pragma once
#include <fstream>
#include <iostream>
#include "SceneAsset.hpp"
#include "test_utils.hpp"

// T1.1 -parse the real Unity export and assert key values survive the trip.
// Run from the repo root so the relative asset paths resolve.
inline void test_scene_asset()
{
    const std::string root = "assets/unity_export/ValidationScene/";

    // ---- scene.json ----
    std::ifstream sf(root + "scene.json");
    CHECK(sf.is_open() && "ValidationScene/scene.json not found -export it from Unity first");
    asset::SceneAsset scene = nlohmann::json::parse(sf).get<asset::SceneAsset>();

    CHECK(scene.name == "ValidationScene");
    // Ground, Cube, Sphere, Capsule, Textured (the M2 textured sphere).
    CHECK(scene.objects.size() == 5);

    // Ground: scale (2,1,2) -> localToWorld diag (2,1,2); worldToLocal diag (0.5,1,0.5).
    const asset::ObjectAsset& ground = scene.objects[0];
    CHECK(ground.name == "Ground");
    CHECK(ground.materials.size() == 1);
    CHECK(equal(ground.matrix[0][0], 2.0f));
    CHECK(equal(ground.matrix[2][2], 2.0f));
    CHECK(equal(ground.worldToLocal[0][0], 0.5f));

    // Cube: localToWorld translation column = (-2.5, 0.5, 0).
    const asset::ObjectAsset& cube = scene.objects[1];
    CHECK(cube.name == "Cube");
    CHECK(equal(cube.matrix[0][3], -2.5f));
    CHECK(equal(cube.matrix[1][3],  0.5f));

    // Textured (M2 sphere): scale 1.5, raised to y=2.4.
    const asset::ObjectAsset& textured = scene.objects[4];
    CHECK(textured.name == "Textured");
    CHECK(equal(textured.matrix[0][0], 1.5f));
    CHECK(equal(textured.matrix[1][3], 2.4f));

    // Camera projection (verbatim OpenGL form): aspect forced to 960/540=1.7778
    // by the exporter, so m00 = m11/aspect = 1.73205/1.7778 = 0.97428.
    const asset::CameraAsset& cam = scene.camera;
    CHECK(equal(cam.projection[0][0], 0.974278569f, 0.001f));
    CHECK(equal(cam.projection[1][1], 1.73205f, 0.001f));
    CHECK(equal(cam.projection[3][2], -1.0f));
    CHECK(equal(cam.worldToCamera[0][0], 1.0f));
    CHECK(equal(cam.near, 0.3f));

    // Main light direction.
    CHECK(equal(scene.light.direction.x, -0.321393818f, 0.001f));
    CHECK(equal(scene.light.direction.y, -0.766044557f, 0.001f));
    CHECK(equal(scene.light.direction.z,  0.5566703f,   0.001f));

    // ---- a material file ----
    std::ifstream mf(root + "materials/Cube_Mat.mat.json");
    CHECK(mf.is_open());
    asset::MaterialAsset mat = nlohmann::json::parse(mf).get<asset::MaterialAsset>();
    CHECK(mat.shaderModel == "Lit");
    CHECK(mat.cull == "back");
    CHECK(mat.surfaceType == "opaque");
    CHECK(equal(mat.baseColor.r, 0.8f, 0.001f));
    CHECK(equal(mat.baseColor.g, 0.2f, 0.01f));
    CHECK(mat.baseMap.empty());

    std::cout << "[PASS] test_scene_asset\n";
}
