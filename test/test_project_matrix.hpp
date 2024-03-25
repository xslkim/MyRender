#pragma once
#include "Render.hpp"
#include "Camera.hpp"
#include "test_utils.hpp"

void test_project_matrix_640_480()
{
	std::ifstream f("assets/project_test_640_480.json");
	if (f.is_open())
	{
		json j = json::parse(f);
		for (auto& item : j) {
			float fov = item["fov"].template get<float>();
			float near = item["near"].template get<float>();
			float far = item["far"].template get<float>();
			float aspect = 640.f / 480.f;

			float m00 = item["m00"].template get<float>();
			float m01 = item["m01"].template get<float>();
			float m02 = item["m02"].template get<float>();
			float m03 = item["m03"].template get<float>();

			float m10 = item["m10"].template get<float>();
			float m11 = item["m11"].template get<float>();
			float m12 = item["m12"].template get<float>();
			float m13 = item["m13"].template get<float>();

			float m20 = item["m20"].template get<float>();
			float m21 = item["m21"].template get<float>();
			float m22 = item["m22"].template get<float>();
			float m23 = item["m23"].template get<float>();

			float m30 = item["m30"].template get<float>();
			float m31 = item["m31"].template get<float>();
			float m32 = item["m32"].template get<float>();
			float m33 = item["m33"].template get<float>();

			Camera cam = Camera(Vec3f(0, 0, 0), Vec3f(0, 0, 0));
			cam.Far = far;
			cam.Fov = fov;
			cam.Near = near;
			cam.Aspect = aspect;
			Render::Get().UpdateProjectMatrix(cam);
			MatrixEqual(gpu::UNITY_MATRIX_P,
				m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33);
		}
	}
}

void test_project_matrix_1920_1080()
{
	std::ifstream f("assets/project_test_1920_1080.json");
	if (f.is_open())
	{
		json j = json::parse(f);
		for (auto& item : j) {
			float fov = item["fov"].template get<float>();
			float near = item["near"].template get<float>();
			float far = item["far"].template get<float>();
			float aspect = 1920.f / 1080.f;

			float m00 = item["m00"].template get<float>();
			float m01 = item["m01"].template get<float>();
			float m02 = item["m02"].template get<float>();
			float m03 = item["m03"].template get<float>();

			float m10 = item["m10"].template get<float>();
			float m11 = item["m11"].template get<float>();
			float m12 = item["m12"].template get<float>();
			float m13 = item["m13"].template get<float>();

			float m20 = item["m20"].template get<float>();
			float m21 = item["m21"].template get<float>();
			float m22 = item["m22"].template get<float>();
			float m23 = item["m23"].template get<float>();

			float m30 = item["m30"].template get<float>();
			float m31 = item["m31"].template get<float>();
			float m32 = item["m32"].template get<float>();
			float m33 = item["m33"].template get<float>();

			Camera cam = Camera(Vec3f(0, 0, 0), Vec3f(0, 0, 0));
			cam.Far = far;
			cam.Fov = fov;
			cam.Near = near;
			cam.Aspect = aspect;
			Render::Get().UpdateProjectMatrix(cam);
			MatrixEqual(gpu::UNITY_MATRIX_P,
				m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33);
		}
	}
}