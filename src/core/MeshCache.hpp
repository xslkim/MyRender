#pragma once
#include <map>
#include <string>
#include "Mesh.hpp"

class MeshCache {
public:
    static MeshCache& Get()
    {
        static MeshCache instance;
        return instance;
    }

    Mesh* GetMesh(const std::string& file_name)
    {
        auto it = _meshes.find(file_name);
        if (it != _meshes.end())
            return it->second;

        Mesh* mesh = new Mesh(file_name);
        _meshes[file_name] = mesh;
        return mesh;
    }

    ~MeshCache() { /* TODO: cleanup */ }

private:
    std::map<std::string, Mesh*> _meshes;
};
