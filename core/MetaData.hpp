#pragma once
#include <string>
#include <Vector.hpp>
#include <json.hpp>


using namespace std;
using namespace nlohmann;

void from_json(const json& j, Color& color)
{
    color.r = j[0];
    color.g = j[1];
    color.b = j[2];
    color.a = j[3];
}

void from_json(const json& j, Vec3f& v)
{
    v.x = j[0];
    v.y = j[1];
    v.z = j[2];
}

struct MaterialData {
    string name;
    string shader; // "SimpleLit.shader",
    bool transparent; // : "opaque",
    string render_face; // : "front",
    Color base_color; // : [1, 1, 1, 1] ,
    string base_map; // : "elfgirl/base_diffuse.tga",
    string normal_map;
    string specular_map;// : null,
    string emission_map;// : null,
    float tiling_x; // : 1,
    float tiling_y; // : 1,
    float offset_x = 0; // : 1,
    float offset_y = 0; // : 1,
    bool alpha_cutoff; // : false
    float metallic;
    float smoothness;
};

void from_json(const json& j, MaterialData& data)
{
    data.render_face = j["render_face"];
    j["base_color"].get_to(data.base_color);
    data.shader = j["shader"];
    data.base_map = j["base_map"];
    data.normal_map = j["normal_map"];
    data.specular_map = j["specular_map"];
    data.emission_map = j["emission_map"];
    data.tiling_x = j["tiling_x"];
    data.tiling_y = j["tiling_y"];
    data.alpha_cutoff = j["alpha_cutoff"];
    if (j.contains("metallic")) 
        data.metallic = j["metallic"];
    if (j.contains("smoothness")) 
        data.smoothness = j["smoothness"];
    if (j.contains("transparent"))
        data.transparent = j["transparent"];
    if (j.contains("offset_x"))
        data.offset_x = j["offset_x"];
    if (j.contains("tiling_y"))
        data.offset_x = j["tiling_y"];
}

struct GameObjectData {
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
    string mesh;
    string material;
};

struct CameraData 
{
    Vec3f position;
    Vec3f rotation;
    Color back_color;
    float fov;
    float near;
    float far;
};

struct LightData
{
    Vec3f position;
    Vec3f rotation;
    Color color;
    float intensity;
    float indirect_mul;
};

void from_json(const json& j, CameraData& cam)
{
    j.at("position").get_to(cam.position);
    j.at("rotation").get_to(cam.rotation);
    j.at("back_color").get_to(cam.back_color);
    cam.fov = j["fov"];
    cam.near = j["near"];
    cam.far = j["far"];
}

void from_json(const json& j, LightData& light)
{
    j.at("position").get_to(light.position);
    j.at("rotation").get_to(light.rotation);
    j.at("color").get_to(light.color);
    light.intensity = j["intensity"];
    light.indirect_mul = j["indirect_mul"];
}


struct SceneData {
    CameraData cameraData;
    LightData lightData;
    vector<GameObjectData> game_objects;
};


void from_json(const json& j, GameObjectData& obj) {
    j["position"].get_to(obj.position);
    j["rotation"].get_to(obj.rotation);
    j["scale"].get_to(obj.scale);
    obj.mesh = j["mesh"];
    obj.material = j["material"];
}

void from_json(const json& j, vector<GameObjectData>& objs) {
    for (int i = 0; i < j.size(); ++i)
    {
        GameObjectData data;
        j[i].get_to(data);
        objs.push_back(data);
    }
}

void from_json(const json& j, SceneData& scene) {
    j.at("camera").get_to(scene.cameraData);
    j.at("light").get_to(scene.lightData);
    j.at("game_objects").get_to(scene.game_objects);
}
