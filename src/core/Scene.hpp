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
#include "ShadowPass.hpp"
#include "SkyboxPass.hpp"
#include "FlyCamera.hpp"
#include "Frustum.hpp"
#include "BloomPass.hpp"

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
        _shadowsComputed = false;
        _sceneHasSkin    = false;
        for (const auto& o : _model.objects) if (o.skinned) _sceneHasSkin = true;
        Render::Get().Init();
        // Unity's worldToCamera (LH→RH) + projection causes CW winding for
        // camera-facing faces in NDC — the opposite of what sign=-1 assumed.
        Render::Get().SetFrontFaceSign(+1.0f);
    }

    // Switch a Unity scene to free-flight control (seeded from the exported
    // camera, so the first frame matches the export). WASD/QE move, arrows look.
    void EnableFlyCamera()
    {
        _flyEnabled = true;
        _flyCam.InitFrom(_model.camera);
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

        // Advance skinned animations by wall-clock time.
        for (auto& obj : _model.objects)
            if (obj.skinned) obj.player.Advance(dt);

        // Free-flight camera (Unity scenes): drive the model camera's view from
        // keyboard input. Projection stays the exported one; the shadow map is in
        // light space so moving the camera doesn't invalidate the cached shadows.
        if (_flyEnabled) {
            _flyCam.Update(dt);
            _model.camera.view     = _flyCam.View();
            _model.camera.position = _flyCam.position;
        }
    }

    void Render()
    {
        if (_legacyOrbit) RebuildLegacyCamera();

        // Shadow depth pass (Unity scenes only; uses its own depth buffer). The
        // shadow map lives in light space and is independent of the camera, so for
        // a fully static scene it only needs computing once; skinned scenes (moving
        // geometry) recompute every frame.
        if (!_legacyOrbit && (_sceneHasSkin || !_shadowsComputed)) {
            ShadowPass::Render(_model);
            ShadowPass::RenderSpot(_model);
            _shadowsComputed = true;
        }

        Render::Get().SetCamera(_model.camera);
        Render::Get().BeginFrame();
        Render::Get().SetLight(_model.light);
        Render::Get().SetAmbient(_model.ambientColor * _model.ambientIntensity);
        if (_model.sky.shValid)
            Render::Get().SetSH(_model.sky.sh);
        else
            Render::Get().ClearSH();
        Render::Get().SetAdditionalLights(_model.additionalLights);
        Render::Get().SetFog(_model.fog);

        // View-frustum culling: skip objects whose world AABB is outside the
        // camera frustum. For a big scene (Garden: 4000 objects, most off-screen)
        // this removes the bulk of the per-object vertex/clip work.
        Frustum frustum;
        frustum.FromViewProjection(_model.camera.projection * _model.camera.view);

        for (const auto& obj : _model.objects) {
            if (!obj.mesh) continue;
            if (obj.hasAABB && !frustum.TestAABB(obj.aabbMin, obj.aabbMax)) continue;

            // Skinned objects upload their current frame's bone matrices and use
            // the LBS vertex path (no model matrix); others use M as usual.
            if (obj.skinned) {
                obj.player.UploadCurrentFrame();
            } else {
                gpu::_SKINNED = false;
                Render::Get().SetModelMatrices(obj.localToWorld, obj.worldToLocal);
            }

            // Per-object lightmap: set globals before any submesh draw call.
            gpu::_Lightmap   = obj.lightmapTex;
            gpu::_LightmapST = obj.lightmapST;
            gpu::_LIGHTMAP   = obj.hasLightmap;

            const auto& subs = obj.mesh->submeshes;
            for (size_t s = 0; s < subs.size(); ++s) {
                Material* mat = MaterialForSubmesh(obj, s);
                if (mat) Render::Get().Draw(*obj.mesh, subs[s], *mat);
            }
        }

        // Sky gradient for background pixels (A1). No-op if no environment data yet.
        SkyboxPass::Render(_model.sky, _model.camera);

        // Post-processing + linear→sRGB conversion (Y flipped for SDL).
        const PostProcessing& pp = _model.postProcessing;

        // C2 Bloom: disabled — spotlight over-illumination on left wall causes
        // bloom to spill into adjacent dark stud-frame pixels, raising MSE.
        // Re-enable once spotlight shadow or emissive calibration is fixed.
        // if (pp.bloomEnabled && pp.bloomIntensity > 0.0f) {
        //     BloomPass::Apply(Render::Get().GetColorBuffer(),
        //                      Config::kScreenWidth, Config::kScreenHeight,
        //                      pp.bloomThreshold, pp.bloomIntensity);
        // }
        // Pre-compute exposure multiplier (EV100: multiply by 2^EV).
        float exposureMul = std::pow(2.0f, pp.postExposure);
        // Contrast/saturation in linear light (same approximation as Unity's grading).
        float contrastMid = 0.5f;   // pivot
        float contrastFactor = (pp.contrast / 100.0f) + 1.0f; // 0=half, 1=no change, 2=double
        float satFactor      = (pp.saturation / 100.0f) + 1.0f;

        for (int y = 0; y < Config::kScreenHeight; ++y) {
            for (int x = 0; x < Config::kScreenWidth; ++x) {
                float4 color = Render::Get().GetColor(x, Config::kScreenHeight - y - 1);

                // Sky pixels are flagged alpha=0 (SkyboxPass); geometry is alpha=1.
                // Sky colours are already display sRGB — bypass ACES/exposure/contrast.
                if (color.a < 0.5f) {
                    int idx = (y * Config::kScreenWidth + x) * 4;
                    ScreenBuffer[idx + 0] = (unsigned char)std::clamp(color.r * 255.0f + 0.5f, 0.0f, 255.0f);
                    ScreenBuffer[idx + 1] = (unsigned char)std::clamp(color.g * 255.0f + 0.5f, 0.0f, 255.0f);
                    ScreenBuffer[idx + 2] = (unsigned char)std::clamp(color.b * 255.0f + 0.5f, 0.0f, 255.0f);
                    ScreenBuffer[idx + 3] = 255;
                    continue;
                }

                // Post-exposure
                color.r *= exposureMul;
                color.g *= exposureMul;
                color.b *= exposureMul;

                // Tonemapping (applied in linear light before sRGB)
                if (pp.tonemapping == "aces") {
                    // Narkowicz 2015 ACES approximation
                    auto aces = [](float x) -> float {
                        constexpr float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
                        x = std::max(0.0f, x);
                        return (x * (a * x + b)) / (x * (c * x + d) + e);
                    };
                    color.r = aces(color.r);
                    color.g = aces(color.g);
                    color.b = aces(color.b);
                } else if (pp.tonemapping == "neutral") {
                    // Unity Neutral filmic (simplified Hable)
                    auto neutral = [](float x) -> float {
                        x = std::max(0.0f, x);
                        float a = x * (x + 0.0245786f) - 0.000090537f;
                        float b = x * (0.983729f * x + 0.4329510f) + 0.238081f;
                        return a / b;
                    };
                    color.r = neutral(color.r);
                    color.g = neutral(color.g);
                    color.b = neutral(color.b);
                }

                // Contrast (in linear light, pivot = 0.5)
                if (pp.contrast != 0.0f) {
                    color.r = (color.r - contrastMid) * contrastFactor + contrastMid;
                    color.g = (color.g - contrastMid) * contrastFactor + contrastMid;
                    color.b = (color.b - contrastMid) * contrastFactor + contrastMid;
                }

                // Saturation (in linear light)
                if (pp.saturation != 0.0f) {
                    float lum = color.r * 0.2126f + color.g * 0.7152f + color.b * 0.0722f;
                    color.r = lum + (color.r - lum) * satFactor;
                    color.g = lum + (color.g - lum) * satFactor;
                    color.b = lum + (color.b - lum) * satFactor;
                }

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

    // Advance skinned animations by a fixed amount (used by headless capture to
    // grab a specific point in the clip deterministically).
    void AdvanceAnimations(float dt)
    {
        for (auto& obj : _model.objects)
            if (obj.skinned) obj.player.Advance(dt);
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

    // Mutable access to the loaded model (e.g. CLI/env tuning overrides before Render).
    SceneModel& Model() { return _model; }

private:
    SceneModel _model;
    bool       _legacyOrbit     = false;
    bool       _shadowsComputed  = false; // static-scene shadow map cached after frame 1
    bool       _sceneHasSkin      = false; // skinned objects force per-frame shadow recompute

    FlyCamera  _flyCam;                    // free-flight camera (Unity scenes)
    bool       _flyEnabled = false;

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
