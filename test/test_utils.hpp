#pragma once
#include <cstdio>
#include <cstdlib>
#include "Matrix.hpp"

// CHECK — an assertion that works in EVERY build config. Plain assert() is a
// no-op under Release (NDEBUG), which silently turned these golden-value tests
// into vacuous passes. CHECK always evaluates, prints a clear message to stderr,
// and exits non-zero (no Windows abort() dialog).
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "[FAIL] %s:%d  CHECK(%s)\n",                \
                         __FILE__, __LINE__, #cond);                         \
            std::exit(1);                                                    \
        }                                                                    \
    } while (0)

#define kMinNum (0.002)
bool equal(float a, float b, float min = kMinNum)
{
	if (abs(a - b) < min)
	{
		return true;
	}
	return false;
}

void MatrixEqual(float4x4 mat,
	float m00, float m01, float m02, float m03,
	float m10, float m11, float m12, float m13,
	float m20, float m21, float m22, float m23,
	float m30, float m31, float m32, float m33)
{
	CHECK(equal(mat[0][0], m00));
	CHECK(equal(mat[0][1], m01));
	CHECK(equal(mat[0][2], m02));
	CHECK(equal(mat[0][3], m03));

	CHECK(equal(mat[1][0], m10));
	CHECK(equal(mat[1][1], m11));
	CHECK(equal(mat[1][2], m12));
	CHECK(equal(mat[1][3], m13));

	CHECK(equal(mat[2][0], m20));
	CHECK(equal(mat[2][1], m21));
	CHECK(equal(mat[2][2], m22));
	CHECK(equal(mat[2][3], m23));

	CHECK(equal(mat[3][0], m30));
	CHECK(equal(mat[3][1], m31));
	CHECK(equal(mat[3][2], m32));
	CHECK(equal(mat[3][3], m33));
}