#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "MaterialCatch.hpp"
#include "MeshCatch.hpp"

class GameObject 
{
public:
	Vec3f Position;
	Vec3f Rotation;
	Vec3f Scale;
	void Load(GameObjectData data)
	{
		Position = data.position;
		Rotation = data.rotation;
		Scale = data.scale;
		material = MaterialCatch::Get().GetMaterial(data.material);
		mesh = MeshCatch::Get().GetMesh(Config::scene_path + data.mesh);
	}

	Material* material;
	Mesh* mesh;
};
