#pragma once
#include <map>

#include "Program.hpp"
#include "MetaData.hpp"
#include "Vertex.hpp"

using namespace std;


class Material 
{
public:
    MaterialData _data;
    virtual void Load(const MaterialData& data) = 0;
    VertexShader* vertex_shader;
    FragmentShader* fragment_shader;
    virtual void InitAttributes(const  Vertex& vertex, Attributes* attributes) const = 0;
    virtual void UpdateGpuParameter()const = 0;
};
