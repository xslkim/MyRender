#pragma once
#include <map>
#include "Program.hpp"
#include "MetaData.hpp"
#include "Vertex.hpp"

class Material
{
public:
    MaterialData    _data;
    VertexShader*   vertex_shader   = nullptr;
    FragmentShader* fragment_shader = nullptr;

    virtual void Load(const MaterialData& data)                                  = 0;
    virtual void InitAttributes(const Vertex& vertex, Attributes* attributes) const = 0;
    virtual void UpdateGpuParameter() const                                      = 0;
};
