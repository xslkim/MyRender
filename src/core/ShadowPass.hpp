#pragma once
#include <algorithm>
#include <utility>
#include "SceneModel.hpp"
#include "Render.hpp"
#include "ShaderGlobal.hpp"

// ---------------------------------------------------------------------------
// ShadowPass — directional-light shadow map for M3.
//
// Architecture: one pass, one map, no cascades.
//   1. ComputeLightVP: fits a right-handed ortho frustum to the scene AABB.
//   2. Render: calls BeginShadowPass + DrawDepthOnly per opaque object.
//
// The resulting gpu::_LightVP and gpu::_ShadowDepth are then read by
// MainLightRealtimeShadow in ShaderFunction.hpp during the main color pass.
// ---------------------------------------------------------------------------
namespace ShadowPass {

// Compute scene world-space AABB by transforming all mesh triangles.
inline std::pair<float3, float3> ComputeSceneAABB(const SceneModel& model)
{
    float3 mn( 1e9f,  1e9f,  1e9f);
    float3 mx(-1e9f, -1e9f, -1e9f);
    bool any = false;
    for (const auto& obj : model.objects) {
        if (!obj.mesh) continue;
        for (const auto& tri : obj.mesh->triangles) {
            for (const auto& vtx : tri) {
                float4 ws = obj.localToWorld *
                    float4(vtx.position.x, vtx.position.y, vtx.position.z, 1.0f);
                mn.x = std::min(mn.x, ws.x); mn.y = std::min(mn.y, ws.y); mn.z = std::min(mn.z, ws.z);
                mx.x = std::max(mx.x, ws.x); mx.y = std::max(mx.y, ws.y); mx.z = std::max(mx.z, ws.z);
                any = true;
            }
        }
    }
    if (!any) return { float3(0,0,0), float3(1,1,1) };
    return { mn, mx };
}

// Build a right-handed view matrix that looks along lightDir (world space).
// Camera convention: looks along -Z in view space, same as Unity worldToCameraMatrix.
inline float4x4 BuildLightView(const float3& lightDir, const float3& eye)
{
    float3 fwd   = vector_normalize(lightDir);
    float3 worldUp = (std::abs(fwd.y) < 0.99f) ? float3(0, 1, 0) : float3(0, 0, 1);
    float3 right = vector_normalize(vector_cross(worldUp, fwd));
    float3 up    = vector_cross(fwd, right);

    return createMatrix4x4<float>(
         right.x,  right.y,  right.z, -dot(right, eye),
         up.x,     up.y,     up.z,    -dot(up,    eye),
        -fwd.x,   -fwd.y,   -fwd.z,   dot(fwd,   eye),
         0.0f,     0.0f,     0.0f,     1.0f);
}

// Orthographic projection, right-handed, NDC z in [-1, 1].
// Frustum: x in [l,r], y in [b,t], view-space z in [-n, -f] (objects in front of camera).
inline float4x4 BuildOrthoProj(float l, float r, float b, float t, float n, float f)
{
    return createMatrix4x4<float>(
        2.0f/(r-l),      0.0f,           0.0f,         -(r+l)/(r-l),
        0.0f,       2.0f/(t-b),          0.0f,         -(t+b)/(t-b),
        0.0f,            0.0f,      -2.0f/(f-n),       -(f+n)/(f-n),
        0.0f,            0.0f,           0.0f,          1.0f);
}

// Fit the ortho frustum to the scene AABB, compute lightVP, write gpu globals.
inline float4x4 ComputeLightVP(const SceneModel& model)
{
    auto [worldMin, worldMax] = ComputeSceneAABB(model);
    float3 center = (worldMin + worldMax) * 0.5f;

    // Place light eye far enough back that the whole scene falls in front of near.
    float3 fwd = vector_normalize(model.light.direction);
    float3 eye = center - fwd * 1000.0f;

    float4x4 lightView = BuildLightView(fwd, eye);

    // Project all 8 AABB corners to light view space and find tight bounds.
    float3 corners[8] = {
        { worldMin.x, worldMin.y, worldMin.z },
        { worldMax.x, worldMin.y, worldMin.z },
        { worldMin.x, worldMax.y, worldMin.z },
        { worldMax.x, worldMax.y, worldMin.z },
        { worldMin.x, worldMin.y, worldMax.z },
        { worldMax.x, worldMin.y, worldMax.z },
        { worldMin.x, worldMax.y, worldMax.z },
        { worldMax.x, worldMax.y, worldMax.z },
    };

    float minX =  1e9f, maxX = -1e9f;
    float minY =  1e9f, maxY = -1e9f;
    float minZ =  1e9f, maxZ = -1e9f;
    for (const auto& c : corners) {
        float4 vs = lightView * float4(c.x, c.y, c.z, 1.0f);
        minX = std::min(minX, vs.x); maxX = std::max(maxX, vs.x);
        minY = std::min(minY, vs.y); maxY = std::max(maxY, vs.y);
        minZ = std::min(minZ, vs.z); maxZ = std::max(maxZ, vs.z);
    }

    // In right-handed view space, objects in front of the camera have z < 0.
    // The ortho near/far (positive scalars) correspond to -minZ and -maxZ.
    // Add a small slack so the AABB exactly on the boundary is inside.
    float near = -maxZ - 0.1f;
    float far  = -minZ + 0.1f;
    if (near < 0.01f) near = 0.01f;
    if (far  <= near)  far  = near + 1.0f;

    float4x4 lightProj = BuildOrthoProj(minX, maxX, minY, maxY, near, far);
    float4x4 lightVP   = lightProj * lightView;

    gpu::_LightVP         = lightVP;
    gpu::_SHADOWS_ENABLED = true;

    return lightVP;
}

// Run the full shadow depth pass: compute light camera, clear depth, draw all opaque objects.
inline void Render(const SceneModel& model)
{
    ComputeLightVP(model);
    ::Render::Get().BeginShadowPass();

    for (const auto& obj : model.objects) {
        if (!obj.mesh) continue;
        for (const auto& sub : obj.mesh->submeshes)
            ::Render::Get().DrawDepthOnly(*obj.mesh, sub, obj.localToWorld);
    }
}

} // namespace ShadowPass
