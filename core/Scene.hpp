#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <iostream>
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
        Render::Get().Init();
    }

    void Update(float /*deltaMs*/)
    {
        auto now      = std::chrono::high_resolution_clock::now();
        float seconds = std::chrono::duration_cast<std::chrono::duration<float>>(now - _lastFrameTime).count();
        if (seconds >= 1.0f) {
            std::cout << "FPS: " << (_frames / seconds) << "\n";
            _frames        = 0;
            _lastFrameTime = now;
        } else {
            ++_frames;
        }
    }

    void Render()
    {
        Render::Get().UpdateViewMatrix(_camera);
        Render::Get().UpdateProjectMatrix(_camera);
        Render::Get().FrameStart(_camera, _light);
        for (auto& go : _gameObjects) {
            Render::Get().UpdateModelMatrix(go);
            Render::Get().Draw(_camera, *go.mesh, *go.material);
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
    Camera                    _camera;
    Light                     _light;
    std::vector<GameObject>   _gameObjects;
    int                       _frames        = 0;
    std::chrono::high_resolution_clock::time_point _lastFrameTime;
};
