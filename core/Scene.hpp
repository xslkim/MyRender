#pragma once
#include "Matrix.hpp"
#include "Render.hpp"
#include "Texture.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "GameObject.hpp"
#include "MetaData.hpp"
#include <vector>
#include <fstream>
#include <chrono>


using namespace std;

class Scene
{
public:
	void Load(const string& file_name)
	{
		std::ifstream f(file_name);
		if (f.is_open())
		{
			json j = json::parse(f);
			_sceneData = j.template get<SceneData>();
			_camera.Load(_sceneData.cameraData);
			_light.Load(_sceneData.lightData);
			for (int i = 0; i < _sceneData.game_objects.size(); ++i)
			{
				GameObject game_obj;
				game_obj.Load(_sceneData.game_objects[i]);
				_gameObjects.push_back(game_obj);
			}
		}
		Render::Get().Init();
	}
	std::chrono::high_resolution_clock::time_point m_lastFrameTime;
	void Update(float delayTime)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto duration = currentTime - m_lastFrameTime;
		float timeInSeconds = std::chrono::duration_cast<std::chrono::duration<float>>(duration).count();
		if (timeInSeconds >= 1.0f) {
			float fps = static_cast<float>(m_frames) / timeInSeconds;
			std::cout << "FPS: " << fps << std::endl;
			m_frames = 0;
			m_lastFrameTime = currentTime;
		}
		else {
			++m_frames;
		}
	}
	int m_frames = 0;
	void Render()
	{
		Render::Get().UpdateViewMatrix(_camera);
		Render::Get().UpdateProjectMatrix(_camera);
		Render::Get().FrameStart(_camera, _light);
		for (int i = 0; i < _gameObjects.size(); ++i)
		{
			GameObject& obj = _gameObjects[i];
			Render::Get().UpdateModelMatrix(obj);
			Render::Get().Draw(_camera, *obj.mesh, *obj.material);
		}
		
		for (int y = 0; y < Config::kScreenHeight; ++y)
		{
			for (int x = 0; x < Config::kScreenWidth; ++x)
			{
				float4 color = Render::Get().GetColor(x, (Config::kScreenHeight-y-1));
				color.r = float_linear2srgb(color.r);
				color.g = float_linear2srgb(color.g);
				color.b = float_linear2srgb(color.b);
				color.a = float_linear2srgb(color.a);
				int index = y * Config::kScreenWidth + x;
				index *= 4;
				ScreenBuffer[index + 0] = (unsigned char)(gpu::saturate(color.r) * 255.0f);
				ScreenBuffer[index + 1] = (unsigned char)(gpu::saturate(color.g) * 255.0f);
				ScreenBuffer[index + 2] = (unsigned char)(gpu::saturate(color.b) * 255.0f);
				ScreenBuffer[index + 3] = (unsigned char)(gpu::saturate(color.a) * 255.0f);
			}
		}
	}

	unsigned char* ScreenBuffer = nullptr;
private:
	SceneData _sceneData;
	Camera _camera;
	Light _light;
	vector<GameObject> _gameObjects;
};