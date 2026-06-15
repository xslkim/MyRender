#pragma once
#include <fstream>
#include <cstdint>
#include <string>
#include <iostream>
#include <filesystem>
#include "Mesh.hpp"
#include "AnimationClip.hpp"
#include "AnimationPlayer.hpp"
#include "SkinnedLitShader.hpp"
#include "Attributes.hpp"
#include "Varyings.hpp"
#include "test_utils.hpp"

// ---------------------------------------------------------------------------
// T4.R4 — verify linear blend skinning end-to-end against hand-computed values,
// without needing a Unity export. We synthesize a tiny skinned .mesh and a
// 2-frame .anim in the exact binary format, load them through the runtime, and
// check posWS = Σ_b w_b·(S[b]·posOS).
// ---------------------------------------------------------------------------
namespace skin_test_detail {

inline void w_u16(std::ofstream& o, uint16_t v) { o.write((char*)&v, 2); }
inline void w_u32(std::ofstream& o, uint32_t v) { o.write((char*)&v, 4); }
inline void w_f32(std::ofstream& o, float v)    { o.write((char*)&v, 4); }

// 1 triangle, 3 verts, 2 bones. Each vertex gets a known (boneIndex,weight).
inline std::string WriteSynthMesh(const std::string& dir)
{
    std::filesystem::create_directories(dir);
    std::string path = dir + "/synth_bar.mesh";
    std::ofstream o(path, std::ios::binary);

    o.write("MRSH", 4);
    w_u16(o, 1);                 // version
    w_u16(o, (uint16_t)(1 << 0)); // flags: hasSkin
    w_u32(o, 3);                 // vertexCount
    w_u32(o, 3);                 // indexCount
    w_u32(o, 1);                 // submeshCount
    w_u32(o, 2);                 // boneCount
    w_u32(o, 0); w_u32(o, 3);    // submesh range [0,3)

    // 3 verts: pos, normal, tangent(4), uv0(2), then skin idx(4 u16)+weight(4 f32)
    struct V { float px,py,pz; int b0,b1; float w0,w1; };
    V vs[3] = {
        { 0.0f, 0.0f, 0.0f,  0,1, 1.0f, 0.0f },  // fully bone0
        { 0.0f, 1.0f, 0.0f,  0,1, 0.5f, 0.5f },  // half/half
        { 0.0f, 2.0f, 0.0f,  0,1, 0.0f, 1.0f },  // fully bone1
    };
    for (auto& v : vs) {
        w_f32(o, v.px); w_f32(o, v.py); w_f32(o, v.pz);     // position
        w_f32(o, 0); w_f32(o, 0); w_f32(o, 1);              // normal +Z
        w_f32(o, 1); w_f32(o, 0); w_f32(o, 0); w_f32(o, 1); // tangent
        w_f32(o, 0); w_f32(o, 0);                            // uv0
        w_u16(o, (uint16_t)v.b0); w_u16(o, (uint16_t)v.b1);
        w_u16(o, 0); w_u16(o, 0);
        w_f32(o, v.w0); w_f32(o, v.w1); w_f32(o, 0); w_f32(o, 0);
    }
    w_u32(o, 0); w_u32(o, 1); w_u32(o, 2);                   // indices
    // bindposes (2 bones x 16) — identity; runtime skips them.
    for (int b = 0; b < 2; ++b)
        for (int k = 0; k < 16; ++k) w_f32(o, (k % 5 == 0) ? 1.0f : 0.0f);
    return path;
}

// 2 frames, 2 bones. frame0: both identity. frame1: bone0=I, bone1=translate(2,0,0).
inline std::string WriteSynthAnim(const std::string& dir)
{
    std::filesystem::create_directories(dir);
    std::string path = dir + "/synth_bar.anim";
    std::ofstream o(path, std::ios::binary);

    o.write("MRAN", 4);
    w_u16(o, 1); w_u16(o, 0);    // version, flags
    w_f32(o, 30.0f);             // fps
    w_u32(o, 2);                 // frameCount
    w_u32(o, 2);                 // boneCount

    auto identity = [&] { for (int k = 0; k < 16; ++k) w_f32(o, (k % 5 == 0) ? 1.0f : 0.0f); };
    auto translateX2 = [&] {
        float m[16] = {1,0,0,2, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        for (int k = 0; k < 16; ++k) w_f32(o, m[k]);
    };
    // frame 0
    identity(); identity();
    // frame 1
    identity(); translateX2();
    return path;
}

} // namespace skin_test_detail

inline void test_skinning()
{
    using namespace skin_test_detail;
    const std::string dir = "out/_synth";

    // ---- .mesh skin round-trip ----
    std::string meshPath = WriteSynthMesh(dir);
    Mesh mesh(meshPath);
    CHECK(mesh.skinned == true);
    CHECK(mesh.boneCount == 2);
    CHECK(mesh.triangles.size() == 1);
    const Vertex& mv = mesh.triangles[0][1];        // the half/half vertex
    CHECK(equal(mv.position.y, 1.0f));
    CHECK(mv.boneIndex[0] == 0 && mv.boneIndex[1] == 1);
    CHECK(equal(mv.boneWeight[0], 0.5f));
    CHECK(equal(mv.boneWeight[1], 0.5f));

    // ---- .anim parse ----
    std::string animPath = WriteSynthAnim(dir);
    AnimationClip clip(animPath);
    CHECK(clip.frameCount == 2);
    CHECK(clip.boneCount == 2);
    CHECK(equal(clip.fps, 30.0f));

    AnimationPlayer player;
    player.SetClip(&clip);

    // Run the real skinned vertex shader against the synthesized vertices.
    auto skinPosWS = [&](const Vertex& v) -> float3 {
        Attributes a;
        a.positionOS = float4(v.position, 1);
        a.normalOS   = v.normal;
        a.tangentOS  = v.tangent;
        for (int i = 0; i < 4; ++i) { a.boneIndex[i] = v.boneIndex[i]; a.boneWeight[i] = v.boneWeight[i]; }
        Varyings out;
        gpu::LitSkinnedVertexShader(&a, &out);
        return out.positionWS;
    };

    // Frame 0: both bones identity -> posWS == posOS for every vertex.
    player.time = 0.0f;
    player.UploadCurrentFrame();
    CHECK(gpu::_SKINNED == true);
    CHECK(gpu::_BoneCount == 2);
    {
        float3 p = skinPosWS(mesh.triangles[0][1]); // (0,1,0)
        CHECK(equal(p.x, 0.0f)); CHECK(equal(p.y, 1.0f)); CHECK(equal(p.z, 0.0f));
    }

    // Frame 1: bone1 = translate(+2 x). Check each weight blend.
    player.time = 1.0f / clip.fps;                  // lands exactly on frame 1
    player.UploadCurrentFrame();
    CHECK(player.CurrentFrame() == 1);
    {
        // fully bone0 -> unchanged (0,0,0)
        float3 p0 = skinPosWS(mesh.triangles[0][0]);
        CHECK(equal(p0.x, 0.0f));
        // half/half -> 0.5*(0,1,0) + 0.5*(2,1,0) = (1,1,0)
        float3 p1 = skinPosWS(mesh.triangles[0][1]);
        CHECK(equal(p1.x, 1.0f)); CHECK(equal(p1.y, 1.0f)); CHECK(equal(p1.z, 0.0f));
        // fully bone1 -> (0+2, 2, 0) = (2,2,0)
        float3 p2 = skinPosWS(mesh.triangles[0][2]);
        CHECK(equal(p2.x, 2.0f)); CHECK(equal(p2.y, 2.0f)); CHECK(equal(p2.z, 0.0f));
    }

    // Loop wrap: time = duration should wrap back to frame 0.
    player.time = clip.DurationSeconds();
    player.UploadCurrentFrame();
    CHECK(player.CurrentFrame() == 0);

    std::cout << "[PASS] test_skinning\n";
}
