#pragma once
#include <fstream>
#include <memory>
#include "SceneAsset.hpp"
#include "SceneModel.hpp"
#include "MeshCache.hpp"
#include "MaterialCache.hpp"
#include "Config.hpp"
#include "AnimationClip.hpp"
#include "SkinnedLitShader.hpp"

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

            // Skinned objects: load the baked clip and switch to the LBS vertex
            // path. The .mesh skin block was already read into the vertices.
            bool meshSkinned = ro.mesh && ro.mesh->skinned;
            if (o.skinned && meshSkinned && !o.anim.empty()) {
                ro.clip = std::make_shared<AnimationClip>(Config::scene_path + o.anim);
                ro.player.SetClip(ro.clip.get());
                ro.skinned = true;
                for (auto* m : ro.materials)
                    if (m) m->vertex_shader = gpu::LitSkinnedVertexShader;
            }
            model.objects.push_back(ro);
        }

        // player.clip points at the heap AnimationClip held by the shared_ptr,
        // which is stable across the vector copies above.
        return model;
    }
};
