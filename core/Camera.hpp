#pragma once
#include "MetaData.hpp"

class Camera {
public:
	void Load(const CameraData& data)
	{
		Position = data.position;
		Rotation = data.rotation;
		Near = data.near;
		Far = data.far;
		Fov = data.fov;
		Aspect = (float)Config::kScreenWidth / (float)Config::kScreenHeight;
	}

	Camera() {}

	Camera(Vec3f Pos, Vec3f Rot)
	{
		Position = Pos;
		Rotation = Rot;
	}

	void Update()
	{

	}
	Vec3f Position;
	Vec3f Rotation;


	float Near;
	float Far;
	float Fov;
	float Aspect;

	/*inline Vec3f get_near_plane() const { return Vec3f(); }
	__declspec(property(get = get_near_plane)) Vec3f nearClipPlane;*/
private:
	void UpdateViewMatrix() {
		
	}
	void UpdateProjectMatrix() 
	{
		
	}
};
