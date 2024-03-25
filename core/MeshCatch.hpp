#pragma once

#include "SimpleLitMat.hpp"
#include <fstream>
#include "Mesh.hpp"

class MeshCatch {
public:
    static MeshCatch& Get() {
        static MeshCatch _instance;
        return _instance;
    }

    Mesh* GetMesh(const string& file_name) {
        Mesh* mesh = nullptr;
        auto it = _meshes.find(file_name);
        if (it != _meshes.end())
        {
            mesh = it->second;
        }

        if(mesh == nullptr)
        {
            mesh = new Mesh(file_name);
            _meshes[file_name] = mesh;
        }
        return mesh;
    }



    ~MeshCatch() {/*todo*/ }

private:
   
    map<string, Mesh*> _meshes;

};



