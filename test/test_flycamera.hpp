#pragma once
#include <iostream>
#include "FlyCamera.hpp"
#include "SceneModel.hpp"
#include "Input.hpp"
#include "test_utils.hpp"

// F3 — the fly camera must (1) reproduce the exported view exactly on frame 1
// (it is seeded from that matrix), and (2) move along its own basis on input.
inline void test_flycamera()
{
    // A standard worldToCamera: eye at (0,0,-5) looking down +Z (OpenGL -Z view).
    CameraState cam;
    cam.position = float3(0, 0, -5);
    cam.view = createMatrix4x4<float>(
        1, 0,  0, 0,
        0, 1,  0, 0,
        0, 0, -1, -5,
        0, 0,  0, 1);

    FlyCamera fc;
    fc.InitFrom(cam);

    // (1) Seeded view == exported view, element by element.
    float4x4 v = fc.View();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            CHECK(equal(v[r][c], cam.view[r][c]));

    // (2) Pressing W moves forward (+Z here) by moveSpeed*dt.
    Input::Get().SetKeyDown("W");
    fc.Update(0.1f);
    Input::Get().SetKeyUp("W");
    CHECK(equal(fc.position.z, -5.0f + fc.moveSpeed * 0.1f)); // -5 + 1 = -4
    CHECK(equal(fc.position.x, 0.0f));
    CHECK(equal(fc.position.y, 0.0f));

    // (3) Pressing D moves along +right (+X here).
    Input::Get().SetKeyDown("D");
    fc.Update(0.1f);
    Input::Get().SetKeyUp("D");
    CHECK(equal(fc.position.x, fc.moveSpeed * 0.1f));

    // (4) After a yaw the basis stays orthonormal (forward is unit length).
    Input::Get().SetKeyDown("Left");
    fc.Update(0.1f);
    Input::Get().SetKeyUp("Left");
    float len = vector_length(fc.forward);
    CHECK(equal(len, 1.0f));

    std::cout << "[PASS] test_flycamera\n";
}
