#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Config.hpp"
#include <math.h>

class RenderUtils
{
public:
    // Clip space -> screen space. Also outputs NDC position.
    static float4 ClipPositionToScreenPosition(float4 clipPos, float3& ndcPos)
    {
        ndcPos = clipPos.xyz / clipPos.w;
        return float4(
            (ndcPos.x + 1.0f) * 0.5f * (Config::kScreenWidth  - 1),
            (ndcPos.y + 1.0f) * 0.5f * (Config::kScreenHeight - 1),
            ndcPos.z * 0.5f + 0.5f,
            clipPos.w
        );
    }

    // Efficient barycentric coordinate algorithm:
    // https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
    static float3 BarycentricCoordinate(float2 P, float2 v0, float2 v1, float2 v2)
    {
        float2 v2v0 = v2 - v0;
        float2 v1v0 = v1 - v0;
        float2 v0P  = v0 - P;
        float3 u = vector_cross(float3(v2v0.x, v1v0.x, v0P.x), float3(v2v0.y, v1v0.y, v0P.y));
        if (abs(u.z) < 1) return float3(-1, 1, 1);
        return float3(1 - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    }

    static constexpr float NegativeInfinity = -0.000001f;

    // Sutherland-Hodgman clip planes
    enum class ClipPlane { W_PLANE, X_RIGHT, X_LEFT, Y_TOP, Y_BOTTOM, Z_NEAR, Z_FAR };

    static float get_intersect_ratio(float4 prev, float4 curr, ClipPlane plane)
    {
        switch (plane) {
        case ClipPlane::W_PLANE:  return (prev.w - EPSILON)  / (prev.w - curr.w);
        case ClipPlane::X_RIGHT:  return (prev.w - prev.x)   / ((prev.w - prev.x)   - (curr.w - curr.x));
        case ClipPlane::X_LEFT:   return (prev.w + prev.x)   / ((prev.w + prev.x)   - (curr.w + curr.x));
        case ClipPlane::Y_TOP:    return (prev.w - prev.y)   / ((prev.w - prev.y)   - (curr.w - curr.y));
        case ClipPlane::Y_BOTTOM: return (prev.w + prev.y)   / ((prev.w + prev.y)   - (curr.w + curr.y));
        case ClipPlane::Z_NEAR:   return (prev.w - prev.z)   / ((prev.w - prev.z)   - (curr.w - curr.z));
        case ClipPlane::Z_FAR:    return (prev.w + prev.z)   / ((prev.w + prev.z)   - (curr.w + curr.z));
        default: return 0;
        }
    }

    static bool IsInsidePlane(ClipPlane plane, float4 v)
    {
        switch (plane) {
        case ClipPlane::W_PLANE:  return v.w > EPSILON;
        case ClipPlane::X_RIGHT:  return v.x / v.w <= 1;
        case ClipPlane::X_LEFT:   return v.x / v.w >= -1;
        case ClipPlane::Y_TOP:    return v.y / v.w <= 1;
        case ClipPlane::Y_BOTTOM: return v.y / v.w >= -1;
        case ClipPlane::Z_NEAR:   return v.z / v.w <= 1;
        case ClipPlane::Z_FAR:    return v.z / v.w >= -1;
        default: return false;
        }
    }

    static bool IsVisible(float4 posCS)
    {
        posCS = posCS / posCS.w;
        return posCS.x >= -1 && posCS.x <= 1
            && posCS.y >= -1 && posCS.y <= 1
            && posCS.z >= -1 && posCS.z <= 1;
    }

    enum class BlendMode { None, AlphaBlend, Additive, Multiply, Screen };

    static float4 BlendColors(float4 src, float4 dst, BlendMode mode)
    {
        switch (mode) {
        case BlendMode::None:       return src;
        case BlendMode::AlphaBlend: return src.w * src + (1 - src.w) * dst;
        case BlendMode::Additive:   return src + dst;
        case BlendMode::Multiply:   return src * dst;
        case BlendMode::Screen:     return src + dst - src * dst;
        default:                    return src;
        }
    }

    // Convert Euler rotation to a direction vector
    static float3 RotationToDirection(float rot_x, float rot_y, float rot_z, float3 src_dir)
    {
        float3x3 init = createMatrix3x3<float>(1, 0, 0,  0, 1, 0,  0, 0, -1);

        float rx = rot_x * PI / 180.f;
        float3x3 rotX = createMatrix3x3<float>(
            1,       0,        0,
            0, cos(rx), -sin(rx),
            0, sin(rx),  cos(rx));

        float ry = rot_y * PI / 180.f;
        float3x3 rotY = createMatrix3x3<float>(
            cos(ry), 0, sin(ry),
            0,       1, 0,
           -sin(ry), 0, cos(ry));

        float rz = rot_z * PI / 180.f;
        float3x3 rotZ = createMatrix3x3<float>(
            cos(rz),  sin(rz), 0,
           -sin(rz),  cos(rz), 0,
            0,        0,       1);

        return src_dir * (rotZ * rotX * rotY * init);
    }
};
