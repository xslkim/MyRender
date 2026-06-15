#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"

// View-frustum culling. Planes are extracted from the view-projection matrix
// (Gribb-Hartmann): for clipPos = VP * worldPos with the OpenGL clip volume
// (-w <= x,y,z <= w), each frustum plane is a row-sum of VP. A point is inside a
// plane when a*x + b*y + c*z + d >= 0 (planes need not be normalized for a
// sign-only inside/outside test).
struct Frustum {
    float4 planes[6]; // left, right, bottom, top, near, far

    void FromViewProjection(const float4x4& vp)
    {
        float4 r0(vp[0][0], vp[0][1], vp[0][2], vp[0][3]);
        float4 r1(vp[1][0], vp[1][1], vp[1][2], vp[1][3]);
        float4 r2(vp[2][0], vp[2][1], vp[2][2], vp[2][3]);
        float4 r3(vp[3][0], vp[3][1], vp[3][2], vp[3][3]);
        planes[0] = r3 + r0; // left   ( x >= -w )
        planes[1] = r3 - r0; // right  ( x <=  w )
        planes[2] = r3 + r1; // bottom ( y >= -w )
        planes[3] = r3 - r1; // top    ( y <=  w )
        planes[4] = r3 + r2; // near   ( z >= -w )
        planes[5] = r3 - r2; // far    ( z <=  w )
    }

    // True if the world-space AABB is at least partially inside the frustum.
    // Uses the "positive vertex" test: if the AABB corner farthest along a
    // plane's normal is still outside that plane, the whole box is outside.
    bool TestAABB(const float3& mn, const float3& mx) const
    {
        for (const float4& p : planes) {
            float3 pv(p.x >= 0 ? mx.x : mn.x,
                      p.y >= 0 ? mx.y : mn.y,
                      p.z >= 0 ? mx.z : mn.z);
            if (p.x * pv.x + p.y * pv.y + p.z * pv.z + p.w < 0.0f)
                return false; // fully outside this plane
        }
        return true;
    }
};
