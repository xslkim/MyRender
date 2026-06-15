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

            // World AABB for frustum culling: transform the mesh's 8 object-space
            // corners. Skipped for skinned meshes (their box changes when animated).
            if (ro.mesh && !o.skinned) {
                float3 lo = ro.mesh->aabbMin, hi = ro.mesh->aabbMax;
                float3 wmn( 1e30f,  1e30f,  1e30f);
                float3 wmx(-1e30f, -1e30f, -1e30f);
                for (int k = 0; k < 8; ++k) {
                    float3 c((k & 1) ? hi.x : lo.x,
                             (k & 2) ? hi.y : lo.y,
                             (k & 4) ? hi.z : lo.z);
                    float4 w = ro.localToWorld * float4(c.x, c.y, c.z, 1.0f);
                    wmn = vector_min(wmn, float3(w.x, w.y, w.z));
                    wmx = vector_max(wmx, float3(w.x, w.y, w.z));
                }
                ro.aabbMin = wmn;
                ro.aabbMax = wmx;
                ro.hasAABB = true;
            }

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
