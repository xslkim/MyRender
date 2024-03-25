#pragma once
#include "Render.hpp"
#include "Camera.hpp"
#include <cmath>
#include "test_utils.hpp"
#include <sstream>
#include <iomanip>

std::string toFixedString(float value, int precision = 2) {
	if (Abs(value) < 0.000001)
	{
		value = 0;
	}
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(precision) << value;
	return oss.str();
}

// 使用示例

string GenKey(Vec3f pos, Vec2f uv, Vec3f normal)
{
	string key = toFixedString(pos.x);
	key += "_" + toFixedString(pos.y);
	key += "_" + toFixedString(pos.z);

	key += "_" + toFixedString(uv.x);
	key += "_" + toFixedString(uv.y);
	
	key += "_" + toFixedString(normal.x);
	key += "_" + toFixedString(normal.y);
	key += "_" + toFixedString(normal.z);

	return key;
}

void test_mesh_cube()
{
	//vector<string> keys;
	//map<string, Vec4f> mesh_tangle;

	std::ifstream f("assets/mesh_db_test.json");
	if (f.is_open())
	{
		//Mesh mesh("assets/cube/Box3.obj");
		//Mesh mesh("C:/study/my-render/unity/mycar/Assets/Res/Cube/Box3.obj");
		Mesh mesh("C:/study/my-render/unity/mycar/Assets/Res/db/diablo3_pose.obj");
		//for (int i = 0; i < mesh.triangles.size(); ++i)
		//{
		//	auto triangle = mesh.triangles[i];
		//	assert(triangle.size() == 3);
		//	for (int j = 0; j < triangle.size(); ++j)
		//	{
		//		Vertex v = triangle[j];
		//		string key = GenKey(v.position, v.texcoord, v.normal);
		//		mesh_tangle[key] = v.tangent;
		//	}
		//}

		json j = json::parse(f);
		
		for(int index = 0; index < j.size(); ++index)
		{
			auto item = j[index];
			float pos_x = item["pos_x"].template get<float>();
			float pos_y = item["pos_y"].template get<float>();
			float pos_z = item["pos_z"].template get<float>();

			float normal_x = item["normal_x"].template get<float>();
			float normal_y = item["normal_y"].template get<float>();
			float normal_z = item["normal_z"].template get<float>();

			float uv_x = item["uv_x"].template get<float>();
			float uv_y = item["uv_y"].template get<float>();

			float tangent_x = item["tangent_x"].template get<float>();
			float tangent_y = item["tangent_y"].template get<float>();
			float tangent_z = item["tangent_z"].template get<float>();
			float tangent_w = item["tangent_w"].template get<float>();

			Vec3f pos = mesh.triangles[index / 3][index % 3].position;
			Vec2f uv = mesh.triangles[index / 3][index % 3].texcoord;
			Vec3f normal = mesh.triangles[index / 3][index % 3].normal;
			Vec4f tangle = mesh.triangles[index / 3][index % 3].tangent;
			
			
			assert(equal(pos.x, pos_x));
			assert(equal(pos.y, pos_y));
			assert(equal(pos.z, pos_z));

			assert(equal(uv.x, uv_x));
			assert(equal(uv.y, uv_y));

			assert(equal(normal.x, normal_x));
			assert(equal(normal.y, normal_y));
			assert(equal(normal.z, normal_z));

			assert(equal(tangle.x, tangent_x, 0.03f));
			assert(equal(tangle.y, tangent_y, 0.01f));
			assert(equal(tangle.z, tangent_z, 0.02f));
			assert(equal(tangle.w, tangent_w));
		}
	}
}