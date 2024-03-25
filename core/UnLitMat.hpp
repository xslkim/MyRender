#pragma once

#include "Material.hpp"
#include "UnLitShader.hpp"
using namespace std;

class UnLitMat: public Material
{
public:
    Vec4f basecolor;
    Texture2D* diffuse_map;
    Vec2f tiling = Vec2f(1.0f, 1.0f);
    Vec2f offset = Vec2f(0, 0);

    void Load(const MaterialData& data) override 
    {
        _data = data;
        basecolor = data.base_color;
        if (basecolor.a < 1)
        {
            basecolor.a = float_srgb2linear(basecolor.a);
        }
        diffuse_map = TextureCatch::Get().GetTexture(data.base_map);
        tiling = Vec2f(data.tiling_x, data.tiling_y);
        offset = Vec2f(data.offset_x, data.offset_y);
    };

    void InitAttributes(const  Vertex& vertex, Attributes* attributes) const  override
    {
        attributes->positionOS = float4(vertex.position, 1);
        attributes->texcoord = vertex.texcoord;
        attributes->normalOS = vertex.normal;
        attributes->tangentOS = vertex.tangent;
        //to do æ≤Ã¨π‚’’Ã˘Õºuv staticLightmapUV
    }

    void UpdateGpuParameter()const override
    {
        gpu::_BaseMap = diffuse_map;
        gpu::_Tiling = tiling;
        gpu::_Offset = offset;
        gpu::_BaseColor = basecolor;
        gpu::_Surface = _data.transparent ? 1 : 0;
    }
};




