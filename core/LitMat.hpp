#pragma once
#include "Material.hpp"
#include "Texture.hpp"
#include "LitShader.hpp"

class LitMat : public Material
{
public:
    Texture2D* diffuseMap  = nullptr;
    Texture2D* normalMap   = nullptr;
    Texture2D* specularMap = nullptr;
    Texture2D* emissionMap = nullptr;
    Vec2f      tiling      = Vec2f(1.0f, 1.0f);
    Vec2f      offset      = Vec2f(0.0f, 0.0f);

    void Load(const MaterialData& data) override
    {
        _data       = data;
        diffuseMap  = TextureCache::Get().GetTexture(data.base_map);
        normalMap   = TextureCache::Get().GetTexture(data.normal_map, true);
        specularMap = TextureCache::Get().GetTexture(data.specular_map);
        emissionMap = TextureCache::Get().GetTexture(data.emission_map);
        tiling      = Vec2f(data.tiling_x, data.tiling_y);
        offset      = Vec2f(data.offset_x, data.offset_y);
    }

    void InitAttributes(const Vertex& vertex, Attributes* attributes) const override
    {
        attributes->positionOS = float4(vertex.position, 1);
        attributes->texcoord   = vertex.texcoord;
        attributes->normalOS   = vertex.normal;
        attributes->tangentOS  = vertex.tangent;
    }

    void UpdateGpuParameter() const override
    {
        gpu::_BaseMap    = diffuseMap;
        gpu::_BumpMap    = normalMap;
        gpu::_Tiling     = tiling;
        gpu::_Offset     = offset;
        gpu::_BaseColor  = _data.base_color;
        gpu::_Surface    = _data.transparent ? 1.0f : 0.0f;
        gpu::_Metallic   = float3(_data.metallic, _data.metallic, _data.metallic);
        gpu::_Smoothness = _data.smoothness;
    }
};
