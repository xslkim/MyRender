#pragma once
#include <iostream>
#include "Mesh.hpp"
#include "test_utils.hpp"

// T1.2 — load a real exported cube .mesh and assert structure + verbatim values.
inline void test_mesh_binary()
{
    Mesh cube("assets/unity_export/ValidationScene/meshes/Cube_120.mesh");

    // 24 verts, 36 indices -> 12 triangles, single submesh.
    assert(cube.triangles.size() == 12);
    assert(cube.submeshes.size() == 1);
    assert(cube.submeshes[0].start == 0);
    assert(cube.submeshes[0].count == 12);

    // Triangle 0 (indices 0,2,3) first vertex, read verbatim (no x-flip).
    const Vertex& v0 = cube.triangles[0][0];
    assert(equal(v0.position.x,  0.5f));
    assert(equal(v0.position.y, -0.5f));
    assert(equal(v0.position.z,  0.5f));
    assert(equal(v0.normal.z,    1.0f));
    assert(equal(v0.tangent.x,  -1.0f));
    assert(equal(v0.tangent.w,  -1.0f));

    std::cout << "[PASS] test_mesh_binary\n";
}
