#pragma once
#include <iostream>
#include "Mesh.hpp"
#include "test_utils.hpp"

// T1.2 — load a real exported cube .mesh and assert structure + verbatim values.
inline void test_mesh_binary()
{
    Mesh cube("assets/unity_export/ValidationScene/meshes/Cube_120.mesh");

    // 24 verts, 36 indices -> 12 triangles, single submesh.
    CHECK(cube.triangles.size() == 12);
    CHECK(cube.submeshes.size() == 1);
    CHECK(cube.submeshes[0].start == 0);
    CHECK(cube.submeshes[0].count == 12);

    // Triangle 0 first vertex, read verbatim (no x-flip). This vertex sits on
    // the +X face: position (0.5,-0.5,0.5), normal (1,0,0), tangent (-1,0,0,-1).
    const Vertex& v0 = cube.triangles[0][0];
    CHECK(equal(v0.position.x,  0.5f));
    CHECK(equal(v0.position.y, -0.5f));
    CHECK(equal(v0.position.z,  0.5f));
    CHECK(equal(v0.normal.x,     1.0f));
    CHECK(equal(v0.normal.z,     0.0f));
    CHECK(equal(v0.tangent.x,  -1.0f));
    CHECK(equal(v0.tangent.w,  -1.0f));

    std::cout << "[PASS] test_mesh_binary\n";
}
