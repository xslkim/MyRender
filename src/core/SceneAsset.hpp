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
};

struct SceneAsset {
    std::string              name;
    CameraAsset              camera;
    LightAsset               light;
    float3                   ambientColor     = float3(0, 0, 0);
    float                    ambientIntensity = 1.0f;
    std::vector<ObjectAsset> objects;
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
    for (const auto& jo : j.at("objects")) {
        ObjectAsset o;
        jo.get_to(o);
        s.objects.push_back(o);
    }
}

} // namespace asset
