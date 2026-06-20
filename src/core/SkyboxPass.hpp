#pragma once
#include <cmath>
#include <algorithm>
#include "SceneModel.hpp"
#include "Config.hpp"
#include "Render.hpp"

// ---------------------------------------------------------------------------
// SkyboxPass — fills background pixels (depth == 1.0) with a sky gradient.
//
// Gradient source: scene.json "environment" sky/equator/groundColor (A1).
// If the sky data is not present (older exports), this pass is a no-op and
// the solid backgroundColor from BeginFrame already fills the background.
//
// Ray direction: each pixel → NDC → undo perspective projection → undo view
// rotation (V^T_rot) → world direction → blend by ray.y.
//
// No cubemap sampling here; that's A3.
// ---------------------------------------------------------------------------
namespace SkyboxPass {

// Sample the visual sky gradient. Uses skyboxVisual* colors (actual sky appearance).
// Unity's Procedural sky barely changes colour between 5° and 90° elevation — only
// the very thin horizon band (< 5°) transitions to the horizon/haze colour.
// An exponential blend reproduces this: blend→1 quickly above the horizon so the
// zenith (Top) colour dominates almost everywhere in the upper sky.
inline float3 SampleGradient(const SkyEnvironment& sky, float3 dir)
{
    float t = dir.y;  // -1 (down) … +1 (up)

    // k=15: at 4° (t≈0.07) blend≈0.65, at 12° (t≈0.21) blend≈0.96 → Top dominates
    constexpr float k = 15.0f;
    if (t >= 0.0f) {
        float blend = 1.0f - std::exp(-t * k);
        return sky.skyboxVisualMid * (1.0f - blend) + sky.skyboxVisualTop * blend;
    } else {
        float blend = 1.0f - std::exp(t * k);  // t<0 → exp(t*k)<1
        return sky.skyboxVisualMid * (1.0f - blend) + sky.skyboxVisualBot * blend;
    }
}

inline void Render(const SkyEnvironment& sky, const CameraState& cam)
{
    if (!sky.valid) return;

    const float4x4& P = cam.projection;
    const float4x4& V = cam.view;

    // Perspective: focal lengths from the projection matrix diagonal.
    float fx = P[0][0];  // ndcX = fx * vx / (-vz)
    float fy = P[1][1];  // ndcY = fy * vy / (-vz)

    const int W = Config::kScreenWidth;
    const int H = Config::kScreenHeight;

    ::Render& ren = ::Render::Get();

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (ren.GetDepth(x, y) < 1.0f) continue;  // geometry pixel, skip

            // NDC of pixel centre (rasterizer coords: y=0 at bottom)
            float ndcX = (x + 0.5f) / W * 2.0f - 1.0f;
            float ndcY = (y + 0.5f) / H * 2.0f - 1.0f;

            // View-space direction (camera looks along -Z in view space)
            float vx = ndcX / fx;
            float vy = ndcY / fy;
            float vz = -1.0f;

            // World-space direction: V^T_rot * viewDir
            // V is worldToCamera (row-major): V[row][col] = m[row][col]
            // world = transpose(V_3x3) * (vx, vy, vz)
            float wx = V[0][0]*vx + V[1][0]*vy + V[2][0]*vz;
            float wy = V[0][1]*vx + V[1][1]*vy + V[2][1]*vz;
            float wz = V[0][2]*vx + V[1][2]*vy + V[2][2]*vz;
            float len = std::sqrt(wx*wx + wy*wy + wz*wz);
            if (len < 1e-6f) continue;
            float3 dir(wx/len, wy/len, wz/len);

            float3 c = SampleGradient(sky, dir);
            ren.SetColor(x, y, float4(c.x, c.y, c.z, 1.0f));
        }
    }
}

} // namespace SkyboxPass
