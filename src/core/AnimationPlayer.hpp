#pragma once
#include <algorithm>
#include "AnimationClip.hpp"
#include "ShaderGlobal.hpp"

// ---------------------------------------------------------------------------
// AnimationPlayer — advances a baked AnimationClip and uploads the current
// frame's skinning matrices to the gpu:: bone array consumed by the skinned
// vertex path. v1 picks the nearest frame (no inter-frame blending); the bake
// fps is usually high enough that this looks smooth.
// ---------------------------------------------------------------------------
class AnimationPlayer {
public:
    AnimationClip* clip = nullptr;
    bool  loop = true;
    float time = 0.0f;   // seconds into the clip

    void SetClip(AnimationClip* c) { clip = c; time = 0.0f; }

    // Advance by wall-clock dt (seconds).
    void Advance(float dt)
    {
        if (!clip || clip->frameCount == 0) return;
        time += dt;
        float dur = clip->DurationSeconds();
        if (dur > 0.0f) {
            if (loop)            time = time - dur * std::floor(time / dur);
            else                 time = std::min(time, dur);
        }
    }

    // Frame index for the current time (nearest, clamped/wrapped).
    int CurrentFrame() const
    {
        if (!clip || clip->frameCount == 0) return 0;
        int f = (int)(time * clip->fps + 0.5f);
        if (loop) f %= clip->frameCount;
        else      f = std::min(f, clip->frameCount - 1);
        if (f < 0) f += clip->frameCount;
        return f;
    }

    // Upload the current frame's S[] into gpu globals and enable skinning.
    void UploadCurrentFrame() const
    {
        if (!clip || clip->frameCount == 0) { gpu::_SKINNED = false; return; }
        int f = CurrentFrame();
        int n = std::min(clip->boneCount, gpu::kMaxBones);
        for (int b = 0; b < n; ++b) gpu::_BoneMatrices[b] = clip->frames[f][b];
        gpu::_BoneCount = n;
        gpu::_SKINNED   = true;
    }
};
