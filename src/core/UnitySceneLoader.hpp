#pragma once
#include <fstream>
#include "SceneAsset.hpp"
#include "SceneModel.hpp"
#include "MeshCache.hpp"
#include "MaterialCache.hpp"
#include "Config.hpp"

// ---------------------------------------------------------------------------
// UnitySceneLoader — asset::SceneAsset (verbatim Unity export) -> SceneModel.
// Pure copy of matrices; no handedness fixups (that is the whole design point).
// Config::scene_path must be the exported scene root (ends with '/').
// ---------------------------------------------------------------------------
class UnitySceneLoader {
public:
    static SceneModel Load(const std::string& scene_json_rel = "scene.json")
    {
        std::ifstream f(Config::scene_path + scene_json_rel);
        assert(f.is_open() && "Unity scene.json not found");
        asset::SceneAsset a = nlohmann::json::parse(f).get<asset::SceneAsset>();

        SceneModel model;
        model.camera.view       = a.camera.worldToCamera;
        model.camera.projection = a.camera.projection;
        model.camera.position   = a.camera.position;
        model.camera.near       = a.camera.near;
        model.camera.far        = a.camera.far;

        model.light.direction = a.light.direction;
        model.light.color     = a.light.color;
        model.light.intensity = a.light.intensity;

        model.ambientColor     = a.ambientColor;
        model.ambientIntensity = a.ambientIntensity;

        for (const auto& o : a.objects) {
            RenderObject ro;
            ro.mesh = MeshCache::Get().GetMesh(Config::scene_path + o.mesh);
            for (const auto& mp : o.materials)
                ro.materials.push_back(MaterialCache::Get().GetMaterialFromAsset(mp));
            ro.localToWorld = o.matrix;
            ro.worldToLocal = o.worldToLocal;
            model.objects.push_back(ro);
        }
        return model;
    }
};
