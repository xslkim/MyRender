#pragma once
#include "Matrix.hpp"

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
	assert(equal(mat[0][0], m00));
	assert(equal(mat[0][1], m01));
	assert(equal(mat[0][2], m02));
	assert(equal(mat[0][3], m03));

	assert(equal(mat[1][0], m10));
	assert(equal(mat[1][1], m11));
	assert(equal(mat[1][2], m12));
	assert(equal(mat[1][3], m13));

	assert(equal(mat[2][0], m20));
	assert(equal(mat[2][1], m21));
	assert(equal(mat[2][2], m22));
	assert(equal(mat[2][3], m23));
	
	assert(equal(mat[3][0], m30));
	assert(equal(mat[3][1], m31));
	assert(equal(mat[3][2], m32));
	assert(equal(mat[3][3], m33));
}