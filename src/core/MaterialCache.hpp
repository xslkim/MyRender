#pragma once
#include <map>
#include <string>
#include <fstream>
#include <iostream>
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

    // Unity export path: read a .mat.json (camelCase, asset::MaterialAsset) and
    // build a material. M1 maps Lit + Fallback -> LitMat; SimpleLit/Unlit get a
    // Lit approximation for now (proper mapping is M2). Cached by relative path.
    Material* GetMaterialFromAsset(const std::string& rel_path)
    {
        auto it = _materials.find(rel_path);
        if (it != _materials.end())
            return it->second;

        std::ifstream f(Config::scene_path + rel_path);
        if (!f.is_open()) return nullptr;
        asset::MaterialAsset a = nlohmann::json::parse(f).get<asset::MaterialAsset>();

        // M2: Lit + Fallback render through LitMat. Shader Graph / custom shaders
        // map to "Fallback" and are approximated as Lit (base color/map + whatever
        // standard properties the exporter probed). SimpleLit/Unlit also approximate
        // as Lit for now; a faithful SimpleLit/Unlit path is future work.
        if (a.shaderModel != "Lit")
            std::cout << "[Material] '" << a.name << "' shaderModel=" << a.shaderModel
                      << " -> Lit approximation\n";

        auto* m = new LitMat();
        m->vertex_shader   = gpu::LitVertexShader;
        m->fragment_shader = gpu::LitFragmentShader;
        m->LoadAsset(a);

        _materials[rel_path] = m;
        return m;
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
