#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <cmath>
#include "Matrix.hpp"

// ---------------------------------------------------------------------------
// SyntheticSkinScene — generate a full skinned export folder (scene.json + a
// 2-bone bending bar .mesh/.anim + a ground) WITHOUT Unity, so the runtime skin
// path (loader, AnimationPlayer, LBS vertex shader, skinned shadow) can be
// validated headlessly with --capture-unity. Dev tooling, not shipped content.
// ---------------------------------------------------------------------------
namespace SyntheticSkinScene {

inline void wu16(std::ofstream& o, uint16_t v) { o.write((char*)&v, 2); }
inline void wu32(std::ofstream& o, uint32_t v) { o.write((char*)&v, 4); }
inline void wf(std::ofstream& o, float v)      { o.write((char*)&v, 4); }
inline void wmat(std::ofstream& o, const float4x4& m) {
    for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) wf(o, m[r][c]);
}

// Rotate by deg about Z, pivoting at (0,py,0): T(0,py,0)·Rz·T(0,-py,0).
inline float4x4 BendZ(float deg, float py)
{
    float a = deg * 3.14159265f / 180.0f, s = std::sin(a), c = std::cos(a);
    float4x4 Rz = createMatrix4x4<float>(
        c, -s, 0, 0,
        s,  c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1);
    float4x4 Tp = createMatrix4x4<float>(1,0,0,0, 0,1,0, py, 0,0,1,0, 0,0,0,1);
    float4x4 Tn = createMatrix4x4<float>(1,0,0,0, 0,1,0,-py, 0,0,1,0, 0,0,0,1);
    return Tp * Rz * Tn;
}

inline void WriteBarMesh(const std::string& path)
{
    const int   segs = 16;
    const float H = 4.0f, R = 0.3f, joint = 2.0f;
    const int   rings = segs + 1;
    int corner[4][2] = { {-1,-1}, {1,-1}, {1,1}, {-1,1} };

    std::ofstream o(path, std::ios::binary);
    o.write("MRSH", 4);
    wu16(o, 1); wu16(o, (uint16_t)(1 << 0)); // version, flags=hasSkin
    wu32(o, rings * 4);                        // vertexCount
    wu32(o, segs * 4 * 6);                     // indexCount
    wu32(o, 1);                                // submeshCount
    wu32(o, 2);                                // boneCount
    wu32(o, 0); wu32(o, segs * 4 * 6);         // submesh range

    for (int r = 0; r < rings; ++r) {
        float h  = r / (float)segs * H;
        float w1 = std::min(1.0f, std::max(0.0f, (h - (joint - 1.0f)) / 2.0f));
        for (int c = 0; c < 4; ++c) {
            float nx = (float)corner[c][0], nz = (float)corner[c][1];
            float len = std::sqrt(nx*nx + nz*nz);
            wf(o, nx*R); wf(o, h); wf(o, nz*R);          // position
            wf(o, nx/len); wf(o, 0); wf(o, nz/len);      // normal
            wf(o, 1); wf(o, 0); wf(o, 0); wf(o, 1);      // tangent
            wf(o, c / 4.0f); wf(o, h / H);               // uv0
            wu16(o, 0); wu16(o, 1); wu16(o, 0); wu16(o, 0);      // bone idx
            wf(o, 1.0f - w1); wf(o, w1); wf(o, 0); wf(o, 0);     // bone weight
        }
    }
    for (int r = 0; r < segs; ++r)
        for (int c = 0; c < 4; ++c) {
            uint32_t a = r*4 + c, b = r*4 + (c+1)%4, d = (r+1)*4 + c, e = (r+1)*4 + (c+1)%4;
            wu32(o, a); wu32(o, d); wu32(o, b);
            wu32(o, b); wu32(o, d); wu32(o, e);
        }
    // bindposes: bone0 = I, bone1 = translate(0,-joint,0)
    wmat(o, float4x4());
    wmat(o, createMatrix4x4<float>(1,0,0,0, 0,1,0,-joint, 0,0,1,0, 0,0,0,1));
}

inline void WriteGroundMesh(const std::string& path)
{
    std::ofstream o(path, std::ios::binary);
    o.write("MRSH", 4);
    wu16(o, 1); wu16(o, 0);          // version, flags
    wu32(o, 4); wu32(o, 6); wu32(o, 1); wu32(o, 0);
    wu32(o, 0); wu32(o, 6);
    float S = 10.0f;
    float quad[4][3] = { {-S,0,-S}, {S,0,-S}, {S,0,S}, {-S,0,S} };
    float uv[4][2]   = { {0,0}, {1,0}, {1,1}, {0,1} };
    for (int i = 0; i < 4; ++i) {
        wf(o, quad[i][0]); wf(o, quad[i][1]); wf(o, quad[i][2]);
        wf(o, 0); wf(o, 1); wf(o, 0);
        wf(o, 1); wf(o, 0); wf(o, 0); wf(o, 1);
        wf(o, uv[i][0]); wf(o, uv[i][1]);
    }
    uint32_t idx[6] = { 0, 1, 2, 0, 2, 3 };
    for (uint32_t i : idx) wu32(o, i);
}

inline void WriteBarAnim(const std::string& path)
{
    // A few frames bending 0 -> 45 -> 0 -> -45. Frame 0 is already bent 35° so a
    // single static capture shows the deformation.
    float angles[] = { 35.0f, 45.0f, 0.0f, -45.0f };
    int frames = 4;
    std::ofstream o(path, std::ios::binary);
    o.write("MRAN", 4);
    wu16(o, 1); wu16(o, 0);
    wf(o, 30.0f); wu32(o, frames); wu32(o, 2);
    for (int f = 0; f < frames; ++f) {
        wmat(o, float4x4());              // bone0 = identity
        wmat(o, BendZ(angles[f], 2.0f));  // bone1 bend
    }
}

inline void WriteMat(const std::string& path, const std::string& name, float r, float g, float b)
{
    std::ofstream o(path);
    o << "{\n  \"name\": \"" << name << "\",\n  \"shaderModel\": \"Lit\",\n"
      << "  \"surfaceType\": \"opaque\",\n  \"cull\": \"back\",\n"
      << "  \"baseColor\": [" << r << ", " << g << ", " << b << ", 1],\n"
      << "  \"metallic\": 0.0,\n  \"smoothness\": 0.4\n}\n";
}

// Build the whole folder. Returns nothing; caller points --capture-unity at dir.
inline void Generate(const std::string& dir)
{
    namespace fs = std::filesystem;
    fs::create_directories(dir + "/meshes");
    fs::create_directories(dir + "/materials");
    fs::create_directories(dir + "/anims");

    WriteBarMesh(dir + "/meshes/bar.mesh");
    WriteGroundMesh(dir + "/meshes/ground.mesh");
    WriteBarAnim(dir + "/anims/bar.anim");
    WriteMat(dir + "/materials/bar.mat.json",    "Bar",    0.9f, 0.7f, 0.2f);
    WriteMat(dir + "/materials/ground.mat.json", "Ground", 0.6f, 0.6f, 0.6f);

    // Camera looking at the bar from the front; light angled like ValidationScene.
    // worldToCamera for eye (0,3,-9) looking toward (0,2,0): a simple lookAt with
    // the OpenGL -Z view convention. projection = 60° vfov at 960/540.
    std::ofstream o(dir + "/scene.json");
    o << R"({
  "name": "SyntheticSkin",
  "camera": {
    "position": [0, 2, -9],
    "worldToCameraMatrix": [1,0,0,0, 0,1,0,-2, 0,0,-1,-9, 0,0,0,1],
    "projectionMatrix": [0.974278569,0,0,0, 0,1.73205078,0,0, 0,0,-1.0006001,-0.60018003, 0,0,-1,0],
    "near": 0.3, "far": 1000, "aspect": 1.7777778,
    "backgroundColor": [0.05, 0.06, 0.08, 1]
  },
  "mainLight": {
    "direction": [-0.321393818, -0.766044557, 0.5566703],
    "color": [1, 1, 1], "intensity": 1
  },
  "ambient": { "color": [0.05, 0.06, 0.07], "intensity": 1.0 },
  "objects": [
    {
      "name": "Ground", "mesh": "meshes/ground.mesh",
      "materials": ["materials/ground.mat.json"],
      "matrix": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
      "worldToLocal": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
      "skinned": false, "anim": ""
    },
    {
      "name": "Bar", "mesh": "meshes/bar.mesh",
      "materials": ["materials/bar.mat.json"],
      "matrix": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
      "worldToLocal": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1],
      "skinned": true, "anim": "anims/bar.anim"
    }
  ]
})";
}

} // namespace SyntheticSkinScene
