#pragma once
#include "MetaData.hpp"

class Camera {
public:
    Vec3f Position;
    Vec3f Rotation;
    float Near;
    float Far;
    float Fov;
    float Aspect;

    Camera() {}

    Camera(Vec3f pos, Vec3f rot) : Position(pos), Rotation(rot) {}

    void Load(const CameraData& data)
    {
        Position = data.position;
        Rotation = data.rotation;
        Near     = data.near;
        Far      = data.far;
        Fov      = data.fov;
        Aspect   = (float)Config::kScreenWidth / (float)Config::kScreenHeight;
    }
};
