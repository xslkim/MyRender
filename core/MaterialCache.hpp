#pragma once
#include <map>
#include <string>
#include <fstream>
#include "SimpleLitMat.hpp"
#include "LitMat.hpp"
#include "UnLitMat.hpp"

class MaterialCache {
public:
    static MaterialCache& Get()
    {
        static MaterialCache instance;
        return instance;
    }

    Material* GetMaterial(const std::string& file_name)
    {
        auto it = _materials.find(file_name);
        if (it != _materials.end())
            return it->second;

        return Load(file_name);
    }

    ~MaterialCache() { /* TODO: cleanup */ }

private:
    Material* Load(const std::string& file_name)
    {
        std::ifstream f(Config::scene_path + file_name);
        nlohmann::json j = nlohmann::json::parse(f);
        MaterialData data = j.template get<MaterialData>();
        data.name = file_name;

        Material* mat = nullptr;
        if (data.shader == "SimpleLit") {
            auto* m = new SimpleLitMat();
            m->vertex_shader   = gpu::SimpleLitVertexShader;
            m->fragment_shader = gpu::SimpleLitFragmentShader;
            m->Load(data);
            mat = m;
        }
        else if (data.shader == "Lit") {
            auto* m = new LitMat();
            m->vertex_shader   = gpu::LitVertexShader;
            m->fragment_shader = gpu::LitFragmentShader;
            m->Load(data);
            mat = m;
        }
        else if (data.shader == "UnLit") {
            auto* m = new UnLitMat();
            m->vertex_shader   = gpu::UnLitVertexShader;
            m->fragment_shader = gpu::UnLitFragmentShader;
            m->Load(data);
            mat = m;
        }

        if (mat)
            _materials[file_name] = mat;
        return mat;
    }

    std::map<std::string, Material*> _materials;
};
