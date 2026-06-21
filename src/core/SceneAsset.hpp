#pragma once
#include <string>
#include <vector>
#include "Vector.hpp"
#include "Matrix.hpp"
#include <json.hpp>

// ---------------------------------------------------------------------------
// SceneAsset — the in-memory mirror of the Unity exporter's output
// (see docs/MyRender_AssetFormat.md). These are *plain data*: a faithful,
// 1:1 parse of scene.json and *.mat.json with no renderer logic.
//
// They are intentionally separate from the legacy MetaData.hpp structs:
//   - field names are camelCase to match the Unity export verbatim,
//   - transforms are carried as 4x4 matrices (exported from Unity), not euler,
//   - one material per submesh (a vector), not a single material.
//
// The matrix-based runtime model (SceneModel) is built from these in T1.3.
// ---------------------------------------------------------------------------

namespace asset {

using json = nlohmann::json;

// ---- small array readers (explicit, so there is no ADL clash with the
//      legacy from_json overloads for Vec3f/Color in MetaData.hpp) ----

inline float2 ReadVec2(const json& j) { return float2(j[0], j[1]); }
inline float3 ReadVec3(const json& j) { return float3(j[0], j[1], j[2]); }
inline float4 ReadVec4(const json& j) { return float4(j[0], j[1], j[2], j[3]); }

// 16 floats, row-major, column-vector convention — fed straight into the
// UNITY_MATRIX_* globals. createMatrix4x4 also takes row-major arguments,
// so arr[r*4 + c] maps to m[r][c] directly.
inline float4x4 ReadMat4(const json& j)
{
    return createMatrix4x4<float>(
        j[0],  j[1],  j[2],  j[3],
        j[4],  j[5],  j[6],  j[7],
        j[8],  j[9],  j[10], j[11],
        j[12], j[13], j[14], j[15]);
}

// ---------- structs ----------

struct MaterialAsset {
    std::string name;
    std::string shaderModel = "Lit";   // Lit | SimpleLit | Unlit | Fallback
    std::string sourceShader;
    std::string surfaceType = "opaque"; // opaque | transparent
    std::string cull        = "back";   // back | front | off
    bool        alphaClip   = false;
    float       cutoff      = 0.5f;

    Color       baseColor   = Color(1, 1, 1, 1);
    std::string baseMap;                // sRGB
    float2      tiling      = float2(1, 1);
    float2      offset      = float2(0, 0);

    std::string normalMap;              // linear
    float       normalScale = 1.0f;

    float       metallic    = 0.0f;
    float       smoothness  = 0.5f;
    std::string metallicGlossMap;       // linear; R=metal, A=smooth
    std::string smoothnessChannel = "metallicAlpha"; // or albedoAlpha

    std::string occlusionMap;           // linear
    float       occlusionStrength = 1.0f;

    float3      emissionColor = float3(0, 0, 0);
    std::string emissionMap;            // sRGB

    // Per-material baked lightmap scale: approximates missing baked GI / AO.
    // Multiplied onto the final fragment color. Default (1,1,1) = no change.
    float3 bakedGIColor = float3(1, 1, 1);
};

struct AdditionalLightAsset {
    std::string type      = "point";  // "point" | "spot"
    float3      position  = {};
    float3      color     = { 1, 1, 1 };
    float       intensity = 1.0f;
    float       range     = 10.0f;
    float3      direction = { 0, -1, 0 };
    float       spotAngleOuter = 0.0f;
    float       spotAngleInner = 0.0f;
};

struct CameraAsset {
    float3   position;                  // reference/debug only
    float4x4 worldToCamera;             // -> UNITY_MATRIX_V (verbatim)
    float4x4 projection;                // -> UNITY_MATRIX_P (verbatim)
    float    fovVertical = 60.0f;
    float    near        = 0.3f;
    float    far         = 1000.0f;
    float    aspect      = 1.7777778f;
    bool     orthographic = false;
    float    orthoSize   = 5.0f;
    Color    backgroundColor = Color(0, 0, 0, 1);
};

struct LightAsset {
    float3 direction;                   // world-space direction the light travels
    Color  color    = Color(1, 1, 1, 1);
    float  intensity = 1.0f;
};

struct ObjectAsset {
    std::string              name;
    std::string              mesh;      // relative path to .mesh
    std::vector<std::string> materials; // one per submesh, in submesh order
    float4x4                 matrix;       // localToWorld -> UNITY_MATRIX_M
    float4x4                 worldToLocal;  //            -> UNITY_MATRIX_I_M
    bool                     skinned = false;
    std::string              anim;      // relative path to .anim (only if skinned)
    int                      lightmapIndex       = -1;               // -1 = not lightmapped
    float4                   lightmapScaleOffset = float4(1,1,0,0);  // UV2 → lightmap atlas
};

// Post-processing Volume settings (C1). Optional: older exports default to no-op.
struct PostProcessingAsset {
    std::string tonemapping  = "none";   // none | aces | neutral
    float       postExposure = 0.0f;     // EV100 (ColorAdjustments.postExposure)
    float       contrast     = 0.0f;     // -100..100 (ColorAdjustments.contrast)
    float       saturation   = 0.0f;     // -100..100 (ColorAdjustments.saturation)
    bool        bloomEnabled = false;
    float       bloomThreshold = 1.0f;
    float       bloomIntensity = 0.0f;
};

// Sky gradient for background rendering (A1) and SH for ambient (A2).
// Optional: older scene.json without "environment" leaves these at defaults.
struct SkyEnvironmentAsset {
    std::string ambientMode  = "color";           // color | skybox | trilight
    float3      skyColor     = float3(0, 0, 0);   // ambient irradiance (for lighting)
    float3      equatorColor = float3(0, 0, 0);
    float3      groundColor  = float3(0, 0, 0);
    // Visual skybox colors for background rendering (much brighter than irradiance)
    float3      skyboxVisualTop = float3(0.5f, 0.5f, 0.5f);
    float3      skyboxVisualMid = float3(0.4f, 0.4f, 0.4f);
    float3      skyboxVisualBot = float3(0.1f, 0.1f, 0.1f);
    float       skyboxExposure  = 1.0f;
    float       sh[27]       = {};                // L2 SH9, channel-major (R*9, G*9, B*9)
    bool        shValid      = false;             // true when sh[] was exported
    bool        valid        = false;             // false → fall back to backgroundColor
};

// Realtime fog (Unity RenderSettings.fog). Optional top-level "fog" block.
struct FogAsset {
    bool        enabled = false;
    std::string mode    = "exp2";          // linear | exp | exp2
    float3      color   = float3(0.5f, 0.5f, 0.5f);  // linear
    float       density = 0.0f;            // for exp/exp2
    float       start   = 0.0f;            // for linear
    float       end     = 300.0f;          // for linear
};

struct SceneAsset {
    std::string                       name;
    CameraAsset                       camera;
    LightAsset                        light;
    float3                            ambientColor     = float3(0, 0, 0);
    float                             ambientIntensity = 1.0f;
    SkyEnvironmentAsset               sky;
    FogAsset                          fog;
    PostProcessingAsset               postProcessing;
    std::vector<AdditionalLightAsset> additionalLights;
    std::vector<ObjectAsset>          objects;
    std::vector<std::string>          lightmapPaths;   // baked lightmap TGA paths (relative)
};

// ---------- deserialization (ADL-found via the asset:: types) ----------

inline void from_json(const json& j, MaterialAsset& m)
{
    m.name         = j.value("name", "");
    m.shaderModel  = j.value("shaderModel", "Lit");
    m.sourceShader = j.value("sourceShader", "");
    m.surfaceType  = j.value("surfaceType", "opaque");
    m.cull         = j.value("cull", "back");
    m.alphaClip    = j.value("alphaClip", false);
    m.cutoff       = j.value("cutoff", 0.5f);

    if (j.contains("baseColor")) m.baseColor = ReadVec4(j["baseColor"]);
    m.baseMap = j.value("baseMap", "");
    if (j.contains("tiling")) m.tiling = ReadVec2(j["tiling"]);
    if (j.contains("offset")) m.offset = ReadVec2(j["offset"]);

    m.normalMap   = j.value("normalMap", "");
    m.normalScale = j.value("normalScale", 1.0f);

    m.metallic         = j.value("metallic", 0.0f);
    m.smoothness       = j.value("smoothness", 0.5f);
    m.metallicGlossMap = j.value("metallicGlossMap", "");
    m.smoothnessChannel = j.value("smoothnessChannel", "metallicAlpha");

    m.occlusionMap      = j.value("occlusionMap", "");
    m.occlusionStrength = j.value("occlusionStrength", 1.0f);

    if (j.contains("emissionColor")) m.emissionColor = ReadVec3(j["emissionColor"]);
    m.emissionMap = j.value("emissionMap", "");
    if (j.contains("bakedGIColor")) m.bakedGIColor = ReadVec3(j["bakedGIColor"]);
}

inline void from_json(const json& j, CameraAsset& c)
{
    if (j.contains("position")) c.position = ReadVec3(j["position"]);
    c.worldToCamera = ReadMat4(j.at("worldToCameraMatrix"));
    c.projection    = ReadMat4(j.at("projectionMatrix"));
    c.fovVertical   = j.value("fovVertical", 60.0f);
    c.near          = j.value("near", 0.3f);
    c.far           = j.value("far", 1000.0f);
    c.aspect        = j.value("aspect", 1.7777778f);
    c.orthographic  = j.value("orthographic", false);
    c.orthoSize     = j.value("orthoSize", 5.0f);
    if (j.contains("backgroundColor")) c.backgroundColor = ReadVec4(j["backgroundColor"]);
}

inline void from_json(const json& j, LightAsset& l)
{
    if (j.contains("direction")) l.direction = ReadVec3(j["direction"]);
    if (j.contains("color")) {
        // color is [r,g,b]; keep alpha at 1.
        float3 c = ReadVec3(j["color"]);
        l.color = Color(c.x, c.y, c.z, 1);
    }
    l.intensity = j.value("intensity", 1.0f);
}

inline void from_json(const json& j, ObjectAsset& o)
{
    o.name = j.value("name", "");
    o.mesh = j.value("mesh", "");
    if (j.contains("materials"))
        for (const auto& mp : j["materials"]) o.materials.push_back(mp.get<std::string>());
    o.matrix       = ReadMat4(j.at("matrix"));
    o.worldToLocal = ReadMat4(j.at("worldToLocal"));
    o.skinned      = j.value("skinned", false);
    o.anim         = j.value("anim", "");
    o.lightmapIndex = j.value("lightmapIndex", -1);
    if (j.contains("lightmapScaleOffset")) o.lightmapScaleOffset = ReadVec4(j["lightmapScaleOffset"]);
}

inline void from_json(const json& j, PostProcessingAsset& p)
{
    p.tonemapping   = j.value("tonemapping",    "none");
    p.postExposure  = j.value("postExposure",   0.0f);
    p.contrast      = j.value("contrast",       0.0f);
    p.saturation    = j.value("saturation",     0.0f);
    p.bloomEnabled  = j.value("bloomEnabled",   false);
    p.bloomThreshold = j.value("bloomThreshold", 1.0f);
    p.bloomIntensity = j.value("bloomIntensity", 0.0f);
}

inline void from_json(const json& j, SkyEnvironmentAsset& e)
{
    e.ambientMode  = j.value("ambientMode", "color");
    if (j.contains("skyColor"))     e.skyColor     = ReadVec3(j["skyColor"]);
    if (j.contains("equatorColor")) e.equatorColor = ReadVec3(j["equatorColor"]);
    if (j.contains("skyboxVisualTop")) e.skyboxVisualTop = ReadVec3(j["skyboxVisualTop"]);
    if (j.contains("skyboxVisualMid")) e.skyboxVisualMid = ReadVec3(j["skyboxVisualMid"]);
    if (j.contains("skyboxVisualBot")) e.skyboxVisualBot = ReadVec3(j["skyboxVisualBot"]);
    e.skyboxExposure = j.value("skyboxExposure", 1.0f);
    if (j.contains("groundColor"))  e.groundColor  = ReadVec3(j["groundColor"]);
    if (j.contains("sh") && j["sh"].is_array() && j["sh"].size() == 27) {
        for (int i = 0; i < 27; ++i) e.sh[i] = j["sh"][i].get<float>();
        e.shValid = true;
    }
    e.valid = true;
}

inline void from_json(const json& j, FogAsset& f)
{
    f.enabled = j.value("enabled", false);
    f.mode    = j.value("mode", "exp2");
    if (j.contains("color")) f.color = ReadVec3(j["color"]);
    f.density = j.value("density", 0.0f);
    f.start   = j.value("start", 0.0f);
    f.end     = j.value("end", 300.0f);
}

inline void from_json(const json& j, AdditionalLightAsset& a)
{
    a.type           = j.value("type", "point");
    if (j.contains("position"))  a.position  = ReadVec3(j["position"]);
    if (j.contains("color"))     a.color     = ReadVec3(j["color"]);
    a.intensity      = j.value("intensity", 1.0f);
    a.range          = j.value("range", 10.0f);
    if (j.contains("direction")) a.direction = ReadVec3(j["direction"]);
    a.spotAngleOuter = j.value("spotAngleOuter", 0.0f);
    a.spotAngleInner = j.value("spotAngleInner", 0.0f);
}

inline void from_json(const json& j, SceneAsset& s)
{
    s.name = j.value("name", "");
    j.at("camera").get_to(s.camera);
    j.at("mainLight").get_to(s.light);
    if (j.contains("ambient")) {
        if (j["ambient"].contains("color"))     s.ambientColor     = ReadVec3(j["ambient"]["color"]);
        if (j["ambient"].contains("intensity")) s.ambientIntensity = j["ambient"]["intensity"];
    }
    if (j.contains("environment"))    j["environment"].get_to(s.sky);
    if (j.contains("fog"))            j["fog"].get_to(s.fog);
    if (j.contains("postProcessing")) j["postProcessing"].get_to(s.postProcessing);
    if (j.contains("additionalLights")) {
        for (const auto& jl : j["additionalLights"]) {
            AdditionalLightAsset al; jl.get_to(al); s.additionalLights.push_back(al);
        }
    }
    for (const auto& jo : j.at("objects")) {
        ObjectAsset o;
        jo.get_to(o);
        s.objects.push_back(o);
    }
    if (j.contains("lightmaps"))
        for (const auto& lp : j["lightmaps"]) s.lightmapPaths.push_back(lp.get<std::string>());
}

} // namespace asset
