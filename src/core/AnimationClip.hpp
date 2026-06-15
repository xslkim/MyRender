#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>
#include "Matrix.hpp"

// ---------------------------------------------------------------------------
// AnimationClip — a baked skinning-matrix animation (.anim, see
// docs/MyRender_AssetFormat.md). Each frame stores one skinning matrix per bone:
//   S[frame][bone] = bone.localToWorld(frame) · bindpose[bone]
// which maps a skinned vertex from mesh-local straight to world space. No curve
// evaluation happens at runtime — the bake already did it (design §9.2).
// ---------------------------------------------------------------------------
class AnimationClip {
public:
    float fps        = 30.0f;
    int   frameCount = 0;
    int   boneCount  = 0;

    // frames[f][b] = S for bone b at frame f.
    std::vector<std::vector<float4x4>> frames;

    AnimationClip() = default;
    explicit AnimationClip(const std::string& fileName) { Load(fileName); }

    bool Load(const std::string& fileName)
    {
        std::ifstream in(fileName, std::ios::binary);
        if (in.fail()) { assert(false && "cannot open .anim"); return false; }

        auto u16 = [&] { uint16_t v; in.read((char*)&v, 2); return v; };
        auto u32 = [&] { uint32_t v; in.read((char*)&v, 4); return v; };
        auto f32 = [&] { float v;    in.read((char*)&v, 4); return v; };

        char magic[4];
        in.read(magic, 4);
        assert(std::string(magic, 4) == "MRAN" && "bad .anim magic");

        uint16_t version = u16(); (void)version;
        uint16_t flags   = u16(); (void)flags;
        fps        = f32();
        frameCount = (int)u32();
        boneCount  = (int)u32();

        frames.resize(frameCount);
        for (int f = 0; f < frameCount; ++f) {
            frames[f].resize(boneCount);
            for (int b = 0; b < boneCount; ++b) {
                float m[16];
                for (int k = 0; k < 16; ++k) m[k] = f32();
                frames[f][b] = createMatrix4x4<float>(
                    m[0],  m[1],  m[2],  m[3],
                    m[4],  m[5],  m[6],  m[7],
                    m[8],  m[9],  m[10], m[11],
                    m[12], m[13], m[14], m[15]);
            }
        }
        return true;
    }

    float DurationSeconds() const
    {
        return (fps > 0.0f && frameCount > 0) ? frameCount / fps : 0.0f;
    }
};
