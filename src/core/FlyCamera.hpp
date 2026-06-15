#pragma once
#include <cmath>
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Input.hpp"
#include "SceneModel.hpp"

// ---------------------------------------------------------------------------
// FlyCamera — a free-flight camera for walking around an imported scene.
//
// It is seeded from the exported camera's worldToCamera matrix: the matrix rows
// are the camera's world-space basis (row0 = right, row1 = up, row2 = -forward),
// so we extract them verbatim. That means the very first frame reproduces the
// exported view exactly, and the handedness/convention is inherited (no need to
// re-derive it). Only the VIEW matrix is rebuilt as the camera moves; the
// projection stays the exported one.
//
// Controls: W/S/A/D move, Q/E down/up, arrow keys look (yaw/pitch).
// ---------------------------------------------------------------------------
class FlyCamera {
public:
    float3 position;
    float3 right   = float3(1, 0, 0);
    float3 up      = float3(0, 1, 0);
    float3 forward = float3(0, 0, 1);

    float moveSpeed = 10.0f;  // world units / second
    float turnSpeed = 70.0f;  // degrees / second

    void InitFrom(const CameraState& cam)
    {
        position = cam.position;
        right    = float3( cam.view[0][0],  cam.view[0][1],  cam.view[0][2]);
        up       = float3( cam.view[1][0],  cam.view[1][1],  cam.view[1][2]);
        forward  = float3(-cam.view[2][0], -cam.view[2][1], -cam.view[2][2]);
    }

    void Update(float dt)
    {
        const Input& in = Input::Get();

        // Look: yaw about world up, pitch about the camera's own right axis.
        float a = turnSpeed * dt * (PI / 180.0f);
        if (in.GetKey("Left"))  Yaw(+a);
        if (in.GetKey("Right")) Yaw(-a);
        if (in.GetKey("Up"))    Pitch(+a);
        if (in.GetKey("Down"))  Pitch(-a);

        // Move along the current basis (Q/E are world-vertical so it feels stable).
        float v = moveSpeed * dt;
        if (in.GetKey("W")) position += forward * v;
        if (in.GetKey("S")) position -= forward * v;
        if (in.GetKey("D")) position += right * v;
        if (in.GetKey("A")) position -= right * v;
        if (in.GetKey("E")) position += float3(0, 1, 0) * v;
        if (in.GetKey("Q")) position -= float3(0, 1, 0) * v;
    }

    // Rebuild worldToCamera from the basis + position. With the seeded basis and
    // the exported camera position this equals the exported view on frame 1.
    float4x4 View() const
    {
        return createMatrix4x4<float>(
             right.x,    right.y,    right.z,   -dot(right, position),
             up.x,       up.y,       up.z,      -dot(up, position),
            -forward.x, -forward.y, -forward.z,  dot(forward, position),
             0, 0, 0, 1);
    }

private:
    // Rotate v around a unit axis by angle (Rodrigues' rotation formula).
    static float3 RotateAxis(const float3& v, const float3& axis, float angle)
    {
        float c = std::cos(angle), s = std::sin(angle);
        return v * c + vector_cross(axis, v) * s + axis * (dot(axis, v) * (1 - c));
    }

    // Both rotations move the whole basis as a rigid body (rotate every axis by
    // the same amount), which preserves orthogonality AND the original handedness
    // without assuming a left/right-handed cross-product convention.
    void Yaw(float angle)   // around world up
    {
        float3 worldUp(0, 1, 0);
        forward = vector_normalize(RotateAxis(forward, worldUp, angle));
        right   = vector_normalize(RotateAxis(right,   worldUp, angle));
        up      = vector_normalize(RotateAxis(up,      worldUp, angle));
    }

    void Pitch(float angle) // around the camera's own right axis
    {
        forward = vector_normalize(RotateAxis(forward, right, angle));
        up      = vector_normalize(RotateAxis(up,      right, angle));
    }
};
