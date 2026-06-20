#pragma once
#include <vector>
#include <memory>
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "AnimationClip.hpp"
#include "AnimationPlayer.hpp"

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
    Color    backgroundColor = Color(0, 0, 0, 1);
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

    // Skinning (set when the mesh has a skin block + a baked .anim). The clip is
    // owned here; the player advances it and uploads bone matrices before drawing.
    bool                           skinned = false;
    std::shared_ptr<AnimationClip> clip;
    AnimationPlayer                player;

    // World-space AABB for view-frustum culling. hasAABB is false for skinned
    // objects (the bind-pose box is wrong once animated) so they are never culled.
    float3 aabbMin;
    float3 aabbMax;
    bool   hasAABB = false;

    // Baked lightmap (per-object). Null when not lightmapped.
    Texture2D* lightmapTex = nullptr;
    float4     lightmapST  = float4(1, 1, 0, 0);  // xy=scale, zw=offset into atlas
    bool       hasLightmap = false;
};

struct PostProcessing {
    std::string tonemapping  = "none";  // none | aces | neutral
    float       postExposure = 0.0f;   // EV100
    float       contrast     = 0.0f;   // -100..100
    float       saturation   = 0.0f;   // -100..100
    bool        bloomEnabled   = false;
    float       bloomThreshold = 1.0f;
    float       bloomIntensity = 0.0f;
};

struct SkyEnvironment {
    float3 skyColor     = float3(0, 0, 0);   // ambient irradiance (for lighting)
    float3 equatorColor = float3(0, 0, 0);
    float3 groundColor  = float3(0, 0, 0);
    float3 skyboxVisualTop = float3(0.5f, 0.5f, 0.5f);  // visual background color (sky)
    float3 skyboxVisualMid = float3(0.4f, 0.4f, 0.4f);  // visual background color (horizon)
    float3 skyboxVisualBot = float3(0.1f, 0.1f, 0.1f);  // visual background color (ground)
    float  skyboxExposure  = 1.0f;
    float  sh[27]          = {};   // L2 SH9, channel-major (R*9, G*9, B*9)
    bool   shValid         = false;
    bool   valid           = false;
};

struct AdditionalLight {
    bool   isSpot    = false;
    float3 position  = {};
    float3 color     = { 1, 1, 1 };  // pre-multiplied by intensity
    float  range     = 10.0f;
    float3 direction = { 0, -1, 0 };
    float  spotAngleOuter = 0.0f;    // degrees
    float  spotAngleInner = 0.0f;
};

struct SceneModel {
    CameraState                    camera;
    LightState                     light;
    float3                         ambientColor     = float3(0, 0, 0);
    float                          ambientIntensity = 1.0f;
    SkyEnvironment                 sky;
    PostProcessing                 postProcessing;
    std::vector<AdditionalLight>   additionalLights;
    std::vector<RenderObject>      objects;
};
