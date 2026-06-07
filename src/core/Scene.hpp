#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <iostream>
#include <cmath>
#include "Render.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Gameobject.hpp"
#include "MetaData.hpp"

class Scene
{
public:
    unsigned char* ScreenBuffer = nullptr;

    void Load(const std::string& file_name)
    {
        std::ifstream f(file_name);
        if (!f.is_open()) return;
        nlohmann::json j = nlohmann::json::parse(f);
        SceneData data   = j.template get<SceneData>();
        _camera.Load(data.cameraData);
        _light.Load(data.lightData);
        for (const auto& goData : data.game_objects) {
            GameObject go;
            go.Load(goData);
            _gameObjects.push_back(go);
        }

        // Derive orbit parameters from the initial camera pose in the JSON.
        //
        // Convention used by UpdateViewMatrix:
        //   world forward = (sin(ry), ..., cos(rx)*cos(ry))
        // For a camera at XZ angle alpha looking at the origin:
        //   sin(ry) = -sin(alpha)  =>  ry = alpha + 180 degrees
        // So: rotYOffset = rotation.y - alpha * (180/PI)
        //
        _orbitRadius    = std::sqrt(_camera.Position.x * _camera.Position.x +
                                    _camera.Position.z * _camera.Position.z);
        _orbitHeight    = _camera.Position.y;
        _orbitAngle     = std::atan2(_camera.Position.x, _camera.Position.z); // radians
        _orbitRotYOffset = _camera.Rotation.y - _orbitAngle * (180.0f / PI);

        _lastUpdateTime = std::chrono::high_resolution_clock::now();
        Render::Get().Init();
    }

    void Update(float /*deltaMs*/)
    {
        auto  now = std::chrono::high_resolution_clock::now();
        float dt  = std::chrono::duration_cast<std::chrono::duration<float>>(now - _lastUpdateTime).count();
        _lastUpdateTime = now;
        dt = std::min(dt, 0.1f); // cap first-frame spike

        // FPS display (once per second)
        _fpsAccum += dt;
        ++_fpsFrames;
        if (_fpsAccum >= 1.0f) {
            std::cout << "FPS: " << static_cast<int>(_fpsFrames / _fpsAccum) << "\n";
            _fpsFrames = 0;
            _fpsAccum  = 0.0f;
        }

        // Orbit: advance angle then rebuild camera position and Y rotation.
        // X and Z positions trace a circle of _orbitRadius around the origin.
        // _orbitRotYOffset keeps the look-at direction aligned with the orbit.
        _orbitAngle += _orbitSpeed * dt;

        _camera.Position.x = _orbitRadius * std::sin(_orbitAngle);
        _camera.Position.y = _orbitHeight;
        _camera.Position.z = _orbitRadius * std::cos(_orbitAngle);
        _camera.Rotation.y = _orbitAngle * (180.0f / PI) + _orbitRotYOffset;
    }

    // --- Capture helpers (used by the headless --capture screenshot path) ---
    Camera& GetCamera() { return _camera; }
    Light&  GetLight()  { return _light; }
    float   GetOrbitAngleDeg() const { return _orbitAngle * (180.0f / PI); }

    // Place the orbit camera at an absolute angle (degrees) facing the origin.
    void SetOrbitAngleDeg(float deg)
    {
        _orbitAngle = deg * (PI / 180.0f);
        _camera.Position.x = _orbitRadius * std::sin(_orbitAngle);
        _camera.Position.y = _orbitHeight;
        _camera.Position.z = _orbitRadius * std::cos(_orbitAngle);
        _camera.Rotation.y = _orbitAngle * (180.0f / PI) + _orbitRotYOffset;
    }

    void Render()
    {
        Render::Get().UpdateViewMatrix(_camera);
        Render::Get().UpdateProjectMatrix(_camera);
        Render::Get().FrameStart(_camera, _light);
        for (auto& go : _gameObjects) {
            Render::Get().UpdateModelMatrix(go);
            Render::Get().Draw(*go.mesh, *go.material);
        }

        // Convert linear float color buffer -> sRGB bytes for SDL
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

private:
    Camera                  _camera;
    Light                   _light;
    std::vector<GameObject> _gameObjects;

    // FPS counter
    float _fpsAccum  = 0.0f;
    int   _fpsFrames = 0;

    // Per-frame wall-clock time (used for smooth animation delta)
    std::chrono::high_resolution_clock::time_point _lastUpdateTime;

    // Orbit animation state
    float _orbitRadius     = 6.0f;   // horizontal distance from car center
    float _orbitHeight     = 2.6f;   // camera Y (stays fixed during orbit)
    float _orbitAngle      = 0.0f;   // current angle in radians
    float _orbitRotYOffset = 180.0f; // constant offset so camera always faces center
    float _orbitSpeed      = 0.5f;   // radians/second; full circle ~12.6s
};
