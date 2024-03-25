#pragma once
#include <string>
#include "Vector.hpp"
#include "MetaData.hpp"

using namespace std;

class Light
{
public:
	Vec3f Position;
	Vec3f Rotation;
	Color Color;
	float Intensity;
	float Indirect_mul;

	void Load(const LightData& data)
	{
		Position = data.position;
		Rotation = data.rotation;
		Color = data.color;
		Intensity = data.intensity;
		Indirect_mul = data.indirect_mul;
	}
};