#pragma once
#include <fstream>
#include "MetaData.hpp"
#include "SceneModel.hpp"
#include "LegacyTransforms.hpp"
#include "RenderUtils.hpp"
#include "MeshCache.hpp"
#include "MaterialCache.hpp"
#include "Config.hpp"

// ---------------------------------------------------------------------------
// LegacySceneLoader — adapter that turns the old hand-authored scene JSON
// (TRS + euler, single material per object, .obj meshes) into the same
// matrix-based SceneModel the Unity path produces. All euler math is confined
// to LegacyTransforms; the renderer never sees a TRS again.
//
// The camera pose is also returned (out param) so the caller can keep the
// tutorial orbit animation alive by re-deriving camera.view each frame.
// ---------------------------------------------------------------------------
class LegacySceneLoader {
public:
    static SceneModel Load(const std::string& json_path, CameraData& outCamera)
    {
        std::ifstream f(json_path);
        assert(f.is_open() && "legacy scene json not found");
        SceneData data = nlohmann::json::parse(f).get<SceneData>();
        outCamera = data.cameraData;

        SceneModel model;
        const float aspect = (float)Config::kScreenWidth / (float)Config::kScreenHeight;

        model.camera.view       = LegacyTransforms::BuildView(data.cameraData.position, data.cameraData.rotation);
        model.camera.projection = LegacyTransforms::BuildProjection(data.cameraData.fov, aspect,
                                                                    data.cameraData.near, data.cameraData.far);
        model.camera.position   = data.cameraData.position;
        model.camera.near       = data.cameraData.near;
        model.camera.far        = data.cameraData.far;

        // Old path set _MainLightPosition = RotationToDirection(rot, +Z). SetLight
        // negates direction, so store -that to reproduce identical lighting.
        float3 oldDir = RenderUtils::RotationToDirection(
            data.lightData.rotation.x, data.lightData.rotation.y, data.lightData.rotation.z, Vec3f(0, 0, 1));
        model.light.direction = -oldDir;
        model.light.color     = data.lightData.color;
        model.light.intensity = data.lightData.intensity;

        for (const auto& go : data.game_objects) {
            RenderObject ro;
            ro.mesh = MeshCache::Get().GetMesh(Config::scene_path + go.mesh);
            ro.materials.push_back(MaterialCache::Get().GetMaterial(go.material));
            LegacyTransforms::BuildModel(go.position, go.rotation, go.scale,
                                         ro.localToWorld, ro.worldToLocal);
            model.objects.push_back(ro);
        }
        return model;
    }
};
