#pragma once
#include "Material.hpp"
#include "Texture.hpp"
#include "LitShader.hpp"
#include "SceneAsset.hpp"

class LitMat : public Material
{
public:
    Texture2D* diffuseMap   = nullptr;
    Texture2D* normalMap    = nullptr;
    Texture2D* specularMap  = nullptr;  // URP _MetallicGlossMap (R=metal, A=smooth)
    Texture2D* occlusionMap = nullptr;
    Texture2D* emissionMap  = nullptr;
    Vec2f      tiling       = Vec2f(1.0f, 1.0f);
    Vec2f      offset       = Vec2f(0.0f, 0.0f);
    float      normalScale       = 1.0f;
    float      occlusionStrength = 1.0f;
    float3     emissionColor     = float3(0, 0, 0);
    bool       smoothnessFromAlbedo = false;

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

    // Build from the Unity export (docs/MyRender_AssetFormat.md): full URP Lit
    // property mapping — base/normal/mask/occlusion/emission + scalar fallbacks.
    void LoadAsset(const asset::MaterialAsset& a)
    {
        _data.name        = a.name;
        _data.transparent = (a.surfaceType == "transparent");
        _data.base_color  = a.baseColor;
        _data.metallic    = a.metallic;
        _data.smoothness  = a.smoothness;

        diffuseMap   = TextureCache::Get().GetTexture(a.baseMap);
        normalMap    = TextureCache::Get().GetTexture(a.normalMap, true);
        specularMap  = TextureCache::Get().GetTexture(a.metallicGlossMap, true);
        occlusionMap = TextureCache::Get().GetTexture(a.occlusionMap, true);
        emissionMap  = TextureCache::Get().GetTexture(a.emissionMap);
        tiling       = a.tiling;
        offset       = a.offset;

        normalScale          = a.normalScale;
        occlusionStrength    = a.occlusionStrength;
        emissionColor        = a.emissionColor;
        smoothnessFromAlbedo = (a.smoothnessChannel == "albedoAlpha");

        cull = a.cull == "off"   ? Cull::Off
             : a.cull == "front" ? Cull::Front
                                  : Cull::Back;
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
        gpu::_Tiling     = tiling;
        gpu::_Offset     = offset;
        gpu::_BaseColor  = _data.base_color;
        gpu::_Surface    = _data.transparent ? 1.0f : 0.0f;
        gpu::_Metallic   = float3(_data.metallic, _data.metallic, _data.metallic);
        gpu::_Smoothness = _data.smoothness;

        // Normal map
        gpu::_BumpMap   = normalMap;
        gpu::_NORMALMAP = (normalMap != nullptr);
        gpu::_BumpScale = normalScale;

        // Metallic/smoothness mask (URP _MetallicGlossMap)
        gpu::_SpecGlossMap = specularMap;
        gpu::_SPECGLOSSMAP = (specularMap != nullptr);
        gpu::_SMOOTHNESS_TEXTURE_ALBEDO_CHANNEL_A = smoothnessFromAlbedo;

        // Occlusion
        gpu::_OcclusionMap      = occlusionMap;
        gpu::_OCCLUSIONMAP      = (occlusionMap != nullptr);
        gpu::_OcclusionStrength = occlusionStrength;

        // Emission (enabled if a map or a non-black color is present)
        bool hasEmission = (emissionMap != nullptr) ||
                           (emissionColor.x + emissionColor.y + emissionColor.z) > 0.0f;
        gpu::_EmissionMap   = emissionMap;
        gpu::_EMISSION      = hasEmission;
        gpu::_EmissionColor = Color(emissionColor.x, emissionColor.y, emissionColor.z, 1.0f);
    }
};
