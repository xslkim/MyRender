#pragma once

#include "Material.hpp"
#include "SimpleLitShader.hpp"
#include "LitShader.hpp"
using namespace std;

class LitMat: public Material
{
public:
    Vec4f basecolor;
    Texture2D* diffuse_map;
    Texture2D* noumal_map;
    Texture2D* specular_map;
    Texture2D* emission_map;
    Vec2f tiling = Vec2f(1.0f, 1.0f);
    Vec2f offset = Vec2f(0, 0);

    void Load(const MaterialData& data) override 
    {
        _data = data;
        diffuse_map = TextureCatch::Get().GetTexture(data.base_map);
        noumal_map = TextureCatch::Get().GetTexture(data.normal_map, true);
        specular_map = TextureCatch::Get().GetTexture(data.specular_map);
        emission_map = TextureCatch::Get().GetTexture(data.emission_map);
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
        gpu::_BumpMap = noumal_map;
        gpu::_Tiling = tiling;
        gpu::_Offset = offset;
        gpu::_BaseColor = _data.base_color;
        gpu::_Surface = _data.transparent ? 1 : 0;

        gpu::_Metallic = float3(_data.metallic, _data.metallic, _data.metallic);
        gpu::_Smoothness = _data.smoothness;
    }
};




