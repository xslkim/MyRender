#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Config.hpp"
#include <math.h>

// ---------------------------------------------------------------------------
// LegacyTransforms — the original hand-authored TRS+euler matrix math, used
// only by the legacy (tutorial car) scene path. The Unity importer never calls
// this: it feeds Unity's matrices verbatim. Keeping this isolated here means the
// renderer core stays purely matrix-based, and the old conventions live in one
// documented place.
//
// Conventions (OpenGL-style, matching the rasterizer): view looks down -Z,
// rotation order Y*X*Z for the model, Rz*Rx*Ry for the view.
// ---------------------------------------------------------------------------
namespace LegacyTransforms {

// localToWorld (M) and its inverse-ish (I_M) from TRS + euler degrees.
inline void BuildModel(const Vec3f& position, const Vec3f& rotation, const Vec3f& scale,
                       float4x4& outM, float4x4& outIM)
{
    float4x4 T = createMatrix4x4<float>(
        1, 0, 0, position.x,
        0, 1, 0, position.y,
        0, 0, 1, position.z,
        0, 0, 0, 1);

    float4x4 S = createMatrix4x4<float>(
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        0, 0, 0, 1);

    float rx = rotation.x * PI / 180.f;
    float4x4 Rx = createMatrix4x4<float>(
        1, 0,        0,         0,
        0, cos(rx), -sin(rx),   0,
        0, sin(rx),  cos(rx),   0,
        0, 0,        0,         1);

    float ry = rotation.y * PI / 180.f;
    float4x4 Ry = createMatrix4x4<float>(
        cos(ry),  0, sin(ry), 0,
        0,        1, 0,       0,
       -sin(ry),  0, cos(ry), 0,
        0,        0, 0,       1);

    float rz = rotation.z * PI / 180.f;
    float4x4 Rz = createMatrix4x4<float>(
        cos(rz), -sin(rz), 0, 0,
        sin(rz),  cos(rz), 0, 0,
        0,        0,       1, 0,
        0,        0,       0, 1);

    float4x4 R  = Ry * Rx * Rz;
    float4x4 RS = R * S;

    outM  = T * R * S;
    outIM = createMatrix4x4<float>(
        1, 0, 0, -position.x,
        0, 1, 0, -position.y,
        0, 0, 1, -position.z,
        0, 0, 0, 1) * RS.Transpose();
}

// worldToCamera (V) from camera position + euler degrees.
inline float4x4 BuildView(const Vec3f& position, const Vec3f& rotation)
{
    float4x4 initV = createMatrix4x4<float>(
        1, 0,  0, 0,
        0, 1,  0, 0,
        0, 0, -1, 0,
        0, 0,  0, 1);

    float rx = rotation.x * PI / 180.f;
    float4x4 Rx = createMatrix4x4<float>(
        1, 0,        0,       0,
        0, cos(rx), -sin(rx), 0,
        0, sin(rx),  cos(rx), 0,
        0, 0,        0,       1);

    float ry = rotation.y * PI / 180.f;
    float4x4 Ry = createMatrix4x4<float>(
        cos(ry), 0, sin(ry), 0,
        0,       1, 0,       0,
       -sin(ry), 0, cos(ry), 0,
        0,       0, 0,       1);

    float rz = rotation.z * PI / 180.f;
    float4x4 Rz = createMatrix4x4<float>(
        cos(rz),  sin(rz), 0, 0,
       -sin(rz),  cos(rz), 0, 0,
        0,        0,       1, 0,
        0,        0,       0, 1);

    float4x4 T = createMatrix4x4<float>(
        1, 0, 0, -position.x,
        0, 1, 0, -position.y,
        0, 0, 1, -position.z,
        0, 0, 0, 1);

    return Rz * Rx * Ry * initV * T;
}

// Perspective projection (OpenGL NDC z in [-1,1]).
inline float4x4 BuildProjection(float fovDeg, float aspect, float near, float far)
{
    float rad        = fovDeg * PI / 180.f;
    float tanHalfFov = tan(rad / 2);
    float fovY       = 1.0f / tanHalfFov;
    float fovX       = fovY / aspect;
    float f          = far;
    float n          = near;

    return createMatrix4x4<float>(
        fovX, 0,    0,                    0,
        0,    fovY, 0,                    0,
        0,    0,   -(f + n) / (f - n),   -(2 * f * n) / (f - n),
        0,    0,   -1,                    0);
}

} // namespace LegacyTransforms
