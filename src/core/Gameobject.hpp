#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "MetaData.hpp"
#include "MaterialCache.hpp"
#include "MeshCache.hpp"

class GameObject
{
public:
    Vec3f     Position;
    Vec3f     Rotation;
    Vec3f     Scale;
    Material* material = nullptr;
    Mesh*     mesh     = nullptr;

    void Load(const GameObjectData& data)
    {
        Position = data.position;
        Rotation = data.rotation;
        Scale    = data.scale;
        material = MaterialCache::Get().GetMaterial(data.material);
        mesh     = MeshCache::Get().GetMesh(Config::scene_path + data.mesh);
    }
};
