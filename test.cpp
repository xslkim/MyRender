
#include "test.h"
using namespace std;
#include "Matrix.hpp"
#include "test/test_view_matrix.hpp"
#include "test/test_model_matrix.hpp"
#include "test/test_project_matrix.hpp"
#include "test/test_mesh_cube.hpp"
#include "test/test_scene_asset.hpp"
#include "test/test_mesh_binary.hpp"
#include "test/test_vertex_transform.hpp"
#include "test/test_material_toggles.hpp"


int main(int argc, char* argv[])
{
	//test_view_matrix();
	//test_model_matrix();
	//test_project_matrix_640_480();
	//test_project_matrix_1920_1080();
	test_mesh_cube();
	test_scene_asset();
	test_mesh_binary();
	test_vertex_transform();
	test_material_toggles();
	return 0;
}
