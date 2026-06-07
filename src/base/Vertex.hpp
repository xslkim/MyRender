#pragma once
#include "Vector.hpp"

struct Vertex {
	Vertex(Vec3f pos, Vec2f uv, Vec3f n) :position(pos), texcoord(uv), normal(n) {}
	Vec3f position;
	Vec2f texcoord;
	Vec3f normal;
	Vec4f tangent;
};
