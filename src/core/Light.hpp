#pragma once
#include "Vector.hpp"
#include "MetaData.hpp"

class Light
{
public:
    Vec3f Position;
    Vec3f Rotation;
    Color Color;
    float Intensity;
    float IndirectMul;

    void Load(const LightData& data)
    {
        Position    = data.position;
        Rotation    = data.rotation;
        Color       = data.color;
        Intensity   = data.intensity;
        IndirectMul = data.indirect_mul;
    }
};
