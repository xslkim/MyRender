#pragma once

#include "SimpleLitMat.hpp"
#include "LitMat.hpp"
#include "UnLitMat.hpp"
#include <fstream>

class MaterialCatch {
public:
    static MaterialCatch& Get() {
        static MaterialCatch _instance;
        return _instance;
    }

    Material* GetMaterial(const string& file_name) {
        Material* mat = GetMat(file_name);
        if(mat == nullptr)
        {
            Load(file_name);
        }
        return GetMat(file_name);
    }

    void Load(const string& file_name)
    {
        std::ifstream f(Config::scene_path + file_name);
        json j = json::parse(f);
        MaterialData data = j.template get<MaterialData>();
        data.name = file_name;
        if (_materials.find(data.name) == _materials.end())
        {
            if (data.shader == "SimpleLit")
            {
                SimpleLitMat* mat = new SimpleLitMat();
                mat->vertex_shader = gpu::SimpleLitVertexShader;
                mat->fragment_shader = gpu::SimpleLitFragmentShader;
                mat->Load(data);
                _materials[data.name] = mat;
            }
            else if (data.shader == "Lit")
            {
                LitMat* mat = new LitMat();
                mat->vertex_shader = gpu::LitVertexShader;
                mat->fragment_shader = gpu::LitFragmentShader;
                mat->Load(data);
                _materials[data.name] = mat;
            }
            else if (data.shader == "UnLit")
            {
                UnLitMat* mat = new UnLitMat();
                mat->vertex_shader = gpu::UnLitVertexShader;
                mat->fragment_shader = gpu::UnLitFragmentShader;
                mat->Load(data);
                _materials[data.name] = mat;
            }
        }
    }

    ~MaterialCatch() {/*todo*/ }

private:
    Material* GetMat(const string& file_name)
    {
        auto it = _materials.find(file_name);
        if (it != _materials.end())
        {
            return it->second;
        }
        return nullptr;
    }
    map<string, Material*> _materials;

};



