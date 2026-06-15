#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <cmath>
#include <algorithm>
#include "Render.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "SceneModel.hpp"
#include "UnitySceneLoader.hpp"
#include "LegacySceneLoader.hpp"
#include "LegacyTransforms.hpp"

// Scene owns a matrix-based SceneModel and feeds it to Render. Two front ends:
//   - LoadUnity:  static snapshot from the Unity exporter (no animation).
//   - LoadLegacy: the tutorial car scene, with the orbit camera kept alive via
//                 a legacy Camera that re-derives the model's view each frame.
class Scene
{
public:
    unsigned char* ScreenBuffer = nullptr;

    // ----- loading -----

    void LoadUnity(const std::string& scene_json_rel = "scene.json")
    {
        _model       = UnitySceneLoader::Load(scene_json_rel);
        _legacyOrbit = false;
        Render::Get().Init();
        Render::Get().SetFrontFaceSign(-1.0f); // verbatim Unity winding
    }

    void LoadLegacy(const std::string& json_path)
    {
        CameraData cam;
        _model = LegacySceneLoader::Load(json_path, cam);
        _camera.Load(cam);
        SetupOrbit();
        _legacyOrbit    = true;
        _lastUpdateTime = std::chrono::high_resolution_clock::now();
        Render::Get().Init();
        Render::Get().SetFrontFaceSign(1.0f); // legacy OBJ winding
    }

    // ----- per-frame -----

    void Update(float /*deltaMs*/)
    {
        auto  now = std::chrono::high_resolution_clock::now();
        float dt  = std::chrono::duration_cast<std::chrono::duration<float>>(now - _lastUpdateTime).count();
        _lastUpdateTime = now;
        dt = std::min(dt, 0.1f);

        _fpsAccum += dt;
        ++_fpsFrames;
        if (_fpsAccum >= 1.0f) {
            std::cout << "FPS: " << static_cast<int>(_fpsFrames / _fpsAccum) << "\n";
            _fpsFrames = 0;
            _fpsAccum  = 0.0f;
        }

        if (_legacyOrbit) {
            _orbitAngle += _orbitSpeed * dt;
            _camera.Position.x = _orbitRadius * std::sin(_orbitAngle);
            _camera.Position.y = _orbitHeight;
            _camera.Position.z = _orbitRadius * std::cos(_orbitAngle);
            _camera.Rotation.y = _orbitAngle * (180.0f / PI) + _orbitRotYOffset;
        }
    }

    void Render()
    {
        if (_legacyOrbit) RebuildLegacyCamera();

        Render::Get().SetCamera(_model.camera);
        Render::Get().BeginFrame();
        Render::Get().SetLight(_model.light);

        for (const auto& obj : _model.objects) {
            if (!obj.mesh) continue;
            Render::Get().SetModelMatrices(obj.localToWorld, obj.worldToLocal);
            const auto& subs = obj.mesh->submeshes;
            for (size_t s = 0; s < subs.size(); ++s) {
                Material* mat = MaterialForSubmesh(obj, s);
                if (mat) Render::Get().Draw(*obj.mesh, subs[s], *mat);
            }
        }

        // Convert linear float color buffer -> sRGB bytes for SDL (Y flipped).
        for (int y = 0; y < Config::kScreenHeight; ++y) {
            for (int x = 0; x < Config::kScreenWidth; ++x) {
                float4 color = Render::Get().GetColor(x, Config::kScreenHeight - y - 1);
                color.r = float_linear2srgb(color.r);
                color.g = float_linear2srgb(color.g);
                color.b = float_linear2srgb(color.b);
                color.a = float_linear2srgb(color.a);
                int idx = (y * Config::kScreenWidth + x) * 4;
                ScreenBuffer[idx + 0] = (unsigned char)(gpu::saturate(color.r) * 255.0f);
                ScreenBuffer[idx + 1] = (unsigned char)(gpu::saturate(color.g) * 255.0f);
                ScreenBuffer[idx + 2] = (unsigned char)(gpu::saturate(color.b) * 255.0f);
                ScreenBuffer[idx + 3] = (unsigned char)(gpu::saturate(color.a) * 255.0f);
            }
        }
    }

    // ----- capture helpers (legacy tutorial path) -----
    Camera& GetCamera() { return _camera; }
    Light&  GetLight()  { return _light; }
    float   GetOrbitAngleDeg() const { return _orbitAngle * (180.0f / PI); }

    void SetOrbitAngleDeg(float deg)
    {
        _orbitAngle = deg * (PI / 180.0f);
        _camera.Position.x = _orbitRadius * std::sin(_orbitAngle);
        _camera.Position.y = _orbitHeight;
        _camera.Position.z = _orbitRadius * std::cos(_orbitAngle);
        _camera.Rotation.y = _orbitAngle * (180.0f / PI) + _orbitRotYOffset;
    }

private:
    SceneModel _model;
    bool       _legacyOrbit = false;

    // Legacy camera/light (only used by the orbit/capture path).
    Camera _camera;
    Light  _light;

    // Re-derive the model camera from the legacy Camera (picks up orbit + any
    // capture-time FOV tweaks), so Render stays purely matrix-driven.
    void RebuildLegacyCamera()
    {
        _model.camera.view       = LegacyTransforms::BuildView(_camera.Position, _camera.Rotation);
        _model.camera.projection = LegacyTransforms::BuildProjection(
            _camera.Fov, _camera.Aspect, _camera.Near, _camera.Far);
        _model.camera.position   = _camera.Position;
        _model.camera.near       = _camera.Near;
        _model.camera.far        = _camera.Far;
    }

    static Material* MaterialForSubmesh(const RenderObject& obj, size_t s)
    {
        if (obj.materials.empty()) return nullptr;
        // Clamp: if fewer materials than submeshes, reuse the last (Unity behavior).
        return obj.materials[std::min(s, obj.materials.size() - 1)];
    }

    void SetupOrbit()
    {
        _orbitRadius     = std::sqrt(_camera.Position.x * _camera.Position.x +
                                     _camera.Position.z * _camera.Position.z);
        _orbitHeight     = _camera.Position.y;
        _orbitAngle      = std::atan2(_camera.Position.x, _camera.Position.z);
        _orbitRotYOffset = _camera.Rotation.y - _orbitAngle * (180.0f / PI);
    }

    // FPS counter
    float _fpsAccum  = 0.0f;
    int   _fpsFrames = 0;
    std::chrono::high_resolution_clock::time_point _lastUpdateTime;

    // Orbit animation state
    float _orbitRadius     = 6.0f;
    float _orbitHeight     = 2.6f;
    float _orbitAngle      = 0.0f;
    float _orbitRotYOffset = 180.0f;
    float _orbitSpeed      = 0.5f;
};
