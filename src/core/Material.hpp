#pragma once
#include <map>
#include "Program.hpp"
#include "MetaData.hpp"
#include "Vertex.hpp"

class Material
{
public:
    // Rasterizer cull mode (resolved from material `cull`; see RasterizeTri).
    enum class Cull { Back, Front, Off };

    MaterialData    _data;
    Cull            cull            = Cull::Back;
    VertexShader*   vertex_shader   = nullptr;
    FragmentShader* fragment_shader = nullptr;

    virtual void Load(const MaterialData& data)                                  = 0;
    virtual void InitAttributes(const Vertex& vertex, Attributes* attributes) const = 0;
    virtual void UpdateGpuParameter() const                                      = 0;
};
