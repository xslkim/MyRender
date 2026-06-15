#pragma once
#include <vector>
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

// ---------------------------------------------------------------------------
// SceneModel — the single, matrix-based runtime scene the renderer consumes.
// Both front-end loaders (UnitySceneLoader, LegacySceneLoader) produce this;
// Render only ever reads matrices from here. No euler, no TRS, no orbit logic.
// ---------------------------------------------------------------------------

struct CameraState {
    float4x4 view;        // -> UNITY_MATRIX_V
    float4x4 projection;  // -> UNITY_MATRIX_P
    float3   position;
    float    near = 0.3f;
    float    far  = 1000.0f;
};

struct LightState {
    float3 direction;                 // world-space direction the light travels
    Color  color     = Color(1, 1, 1, 1);
    float  intensity = 1.0f;
};

struct RenderObject {
    Mesh*                  mesh = nullptr;
    std::vector<Material*> materials;   // one per submesh, in submesh order
    float4x4               localToWorld; // -> UNITY_MATRIX_M
    float4x4               worldToLocal; // -> UNITY_MATRIX_I_M
};

struct SceneModel {
    CameraState               camera;
    LightState                light;
    float3                    ambientColor     = float3(0, 0, 0);
    float                     ambientIntensity = 1.0f;
    std::vector<RenderObject> objects;
};
