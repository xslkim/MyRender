#pragma once
#include "Vector.hpp"

struct Vertex {
	Vertex(Vec3f pos, Vec2f uv, Vec3f n) :position(pos), texcoord(uv), normal(n) {}
	Vec3f position;
	Vec2f texcoord;
	Vec3f normal;
	Vec4f tangent;

	// Skin (only meaningful when the mesh has a skin block; weights sum to 1).
	int   boneIndex[4]  = { 0, 0, 0, 0 };
	float boneWeight[4] = { 0, 0, 0, 0 };
};
