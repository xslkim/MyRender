#pragma once
#include "Render.hpp"
#include "Camera.hpp"
#include "test_utils.hpp"

void test_view_matrix()
{
	std::ifstream f("assets/test_view.json");
	if (f.is_open())
	{
		json j = json::parse(f);
		for (auto& item : j) {
			float pos_x = item["pos_x"].template get<float>();
			float pos_y = item["pos_y"].template get<float>();
			float pos_z = item["pos_z"].template get<float>();

			float rot_x = item["rot_x"].template get<float>();
			float rot_y = item["rot_y"].template get<float>();
			float rot_z = item["rot_z"].template get<float>();

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

			Render::Get().UpdateViewMatrix(Camera(Vec3f(pos_x, pos_y, pos_z), Vec3f(rot_x, rot_y, rot_z)));
			MatrixEqual(gpu::UNITY_MATRIX_V,
				m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33);
		}
	}
}