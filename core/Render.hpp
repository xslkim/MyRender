#pragma once
#include <vector>
#include <algorithm>
#include "Vertex.hpp"
#include "Matrix.hpp"
#include "Texture.hpp"
#include "Config.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "RenderUtils.hpp"
#include "ShaderGlobal.hpp"
#include "Gameobject.hpp"

class Render {
public:
    static Render& Get() {
        static Render instance;
        return instance;
    }

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    void Init()
    {
        ColorBuffer      = new float[kColorBufferSize];
        DepthBuffer      = new float[kDepthBufferSize];
        _defaultColorBuf = new float[kColorBufferSize];
        _defaultDepthBuf = new float[kDepthBufferSize];

        for (int i = 0; i < Config::kScreenWidth * Config::kScreenHeight; ++i) {
            int k = i * 4;
            _defaultColorBuf[k + 0] = _defaultColor.r;
            _defaultColorBuf[k + 1] = _defaultColor.g;
            _defaultColorBuf[k + 2] = _defaultColor.b;
            _defaultColorBuf[k + 3] = _defaultColor.w;
            _defaultDepthBuf[i]     = _defaultDepth;
        }
        ClearColorBuffer();
        ClearDepthBuffer();
    }

    // -------------------------------------------------------------------------
    // Per-frame setup
    // -------------------------------------------------------------------------

    void FrameStart(const Camera& cam, const Light& light)
    {
        ResetFrameBuffers();
        ClearColorBuffer();
        ClearDepthBuffer();

        gpu::_ProjectionParams.x = 1;
        gpu::_ProjectionParams.y = cam.Near;
        gpu::_ProjectionParams.z = cam.Far;
        gpu::_ProjectionParams.w = 1.0f / cam.Far;

        gpu::_WorldSpaceCameraPos  = cam.Position;
        gpu::_ScaledScreenParams.x = Config::kScreenWidth;
        gpu::_ScaledScreenParams.y = Config::kScreenHeight;

        float3 lightDir = RenderUtils::RotationToDirection(
            light.Rotation.x, light.Rotation.y, light.Rotation.z, Vec3f(0, 0, 1));
        gpu::_MainLightPosition.xyz = lightDir;
        gpu::_MainLightColor        = light.Color;
    }

    // -------------------------------------------------------------------------
    // Matrix uploads
    // -------------------------------------------------------------------------

    void UpdateModelMatrix(const GameObject& obj)
    {
        float4x4 T = createMatrix4x4<float>(
            1, 0, 0, obj.Position.x,
            0, 1, 0, obj.Position.y,
            0, 0, 1, obj.Position.z,
            0, 0, 0, 1);

        float4x4 S = createMatrix4x4<float>(
            obj.Scale.x, 0, 0, 0,
            0, obj.Scale.y, 0, 0,
            0, 0, obj.Scale.z, 0,
            0, 0, 0, 1);

        float rx = obj.Rotation.x * PI / 180.f;
        float4x4 Rx = createMatrix4x4<float>(
            1, 0,        0,         0,
            0, cos(rx), -sin(rx),   0,
            0, sin(rx),  cos(rx),   0,
            0, 0,        0,         1);

        float ry = obj.Rotation.y * PI / 180.f;
        float4x4 Ry = createMatrix4x4<float>(
            cos(ry),  0, sin(ry), 0,
            0,        1, 0,       0,
           -sin(ry),  0, cos(ry), 0,
            0,        0, 0,       1);

        float rz = obj.Rotation.z * PI / 180.f;
        float4x4 Rz = createMatrix4x4<float>(
            cos(rz), -sin(rz), 0, 0,
            sin(rz),  cos(rz), 0, 0,
            0,        0,       1, 0,
            0,        0,       0, 1);

        float4x4 R  = Ry * Rx * Rz;
        float4x4 RS = R * S;

        gpu::UNITY_MATRIX_M   = T * R * S;
        gpu::UNITY_MATRIX_I_M = createMatrix4x4<float>(
            1, 0, 0, -obj.Position.x,
            0, 1, 0, -obj.Position.y,
            0, 0, 1, -obj.Position.z,
            0, 0, 0, 1) * RS.Transpose();
    }

    void UpdateViewMatrix(const Camera& camera)
    {
        float4x4 initV = createMatrix4x4<float>(
            1, 0,  0, 0,
            0, 1,  0, 0,
            0, 0, -1, 0,
            0, 0,  0, 1);

        float rx = camera.Rotation.x * PI / 180.f;
        float4x4 Rx = createMatrix4x4<float>(
            1, 0,        0,       0,
            0, cos(rx), -sin(rx), 0,
            0, sin(rx),  cos(rx), 0,
            0, 0,        0,       1);

        float ry = camera.Rotation.y * PI / 180.f;
        float4x4 Ry = createMatrix4x4<float>(
            cos(ry), 0, sin(ry), 0,
            0,       1, 0,       0,
           -sin(ry), 0, cos(ry), 0,
            0,       0, 0,       1);

        float rz = camera.Rotation.z * PI / 180.f;
        float4x4 Rz = createMatrix4x4<float>(
            cos(rz),  sin(rz), 0, 0,
           -sin(rz),  cos(rz), 0, 0,
            0,        0,       1, 0,
            0,        0,       0, 1);

        float4x4 T = createMatrix4x4<float>(
            1, 0, 0, -camera.Position.x,
            0, 1, 0, -camera.Position.y,
            0, 0, 1, -camera.Position.z,
            0, 0, 0, 1);

        gpu::UNITY_MATRIX_V = Rz * Rx * Ry * initV * T;
    }

    void UpdateProjectMatrix(const Camera& camera)
    {
        float rad        = camera.Fov * PI / 180.f;
        float tanHalfFov = tan(rad / 2);
        float fovY       = 1.0f / tanHalfFov;
        float fovX       = fovY / camera.Aspect;
        float f          = camera.Far;
        float n          = camera.Near;

        float4x4 p = createMatrix4x4<float>(
            fovX, 0,    0,                    0,
            0,    fovY, 0,                    0,
            0,    0,   -(f + n) / (f - n),   -(2 * f * n) / (f - n),
            0,    0,   -1,                    0);

        gpu::UNITY_MATRIX_P  = p;
        gpu::UNITY_MATRIX_VP = p * gpu::UNITY_MATRIX_V;
    }

    // -------------------------------------------------------------------------
    // Buffer access
    // -------------------------------------------------------------------------

    float GetDepth(int x, int y) const
    {
        return DepthBuffer[Config::kScreenWidth * y + x];
    }

    void SetDepth(int x, int y, float depth)
    {
        DepthBuffer[Config::kScreenWidth * y + x] = depth;
    }

    inline float4 GetColor(int x, int y) const
    {
        int i = (y * Config::kScreenWidth + x) * 4;
        return float4(ColorBuffer[i], ColorBuffer[i + 1], ColorBuffer[i + 2], ColorBuffer[i + 3]);
    }

    inline void SetColor(int x, int y, float4 color)
    {
        int i = (y * Config::kScreenWidth + x) * 4;
        ColorBuffer[i]     = color.r;
        ColorBuffer[i + 1] = color.g;
        ColorBuffer[i + 2] = color.b;
        ColorBuffer[i + 3] = color.a;
    }

    // -------------------------------------------------------------------------
    // Draw call
    // -------------------------------------------------------------------------

    void Draw(const Camera& /*cam*/, const Mesh& mesh, const Material& mat)
    {
        mat.UpdateGpuParameter();
        for (const auto& tri : mesh.triangles)
            DrawTriangle(tri[0], tri[1], tri[2], mat);
    }

private:
    // -------------------------------------------------------------------------
    // Per-frame allocator for Attributes / Varyings
    // Resets each frame so we avoid per-triangle heap allocation.
    // -------------------------------------------------------------------------
    static const int kBufferSize = 1024 * 1024;

    Attributes* _attributesBuffer = nullptr;
    int         _attributesIdx    = 0;
    Varyings*   _varyingsBuffer   = nullptr;
    int         _varyingsIdx      = 0;

    Attributes* GetOneAttributes()
    {
        if (!_attributesBuffer)
            _attributesBuffer = new Attributes[kBufferSize];
        return &_attributesBuffer[_attributesIdx++];
    }

    Varyings* GetOneVaryings()
    {
        if (!_varyingsBuffer)
            _varyingsBuffer = new Varyings[kBufferSize];
        return &_varyingsBuffer[_varyingsIdx++];
    }

    void ResetFrameBuffers()
    {
        _attributesIdx = 0;
        _varyingsIdx   = 0;
    }

    // -------------------------------------------------------------------------
    // Color / Depth buffers
    // -------------------------------------------------------------------------
    const int kColorBufferSize = Config::kScreenWidth * Config::kScreenHeight * 4;
    const int kDepthBufferSize = Config::kScreenWidth * Config::kScreenHeight;

    float*  ColorBuffer      = nullptr;
    float*  DepthBuffer      = nullptr;
    float*  _defaultColorBuf = nullptr;
    float*  _defaultDepthBuf = nullptr;
    Vec4f   _defaultColor    = { 0, 0, 0, 1 };
    float   _defaultDepth    = 1.0f;

    void ClearColorBuffer() { memcpy(ColorBuffer, _defaultColorBuf, kColorBufferSize * sizeof(float)); }
    void ClearDepthBuffer() { memcpy(DepthBuffer, _defaultDepthBuf, kDepthBufferSize * sizeof(float)); }

    // -------------------------------------------------------------------------
    // Rasterization
    // -------------------------------------------------------------------------

    bool IsFrontFace(float3 v0, float3 v1, float3 v2)
    {
        return vector_cross(v1 - v0, v2 - v0).z < 0;
    }

    // Rasterize a single triangle that is fully in clip space
    void DrawTriangle(const Varyings* v0, const Varyings* v1, const Varyings* v2, const Material& mat)
    {
        float3 ndc0, ndc1, ndc2;
        float4 s0 = RenderUtils::ClipPositionToScreenPosition(v0->positionCS, ndc0);
        float4 s1 = RenderUtils::ClipPositionToScreenPosition(v1->positionCS, ndc1);
        float4 s2 = RenderUtils::ClipPositionToScreenPosition(v2->positionCS, ndc2);

        if (!IsFrontFace(ndc0, ndc1, ndc2)) return;

        int minX = (int)std::min(s0.x, std::min(s1.x, s2.x));
        int minY = (int)std::min(s0.y, std::min(s1.y, s2.y));
        int maxX = (int)std::max(s0.x, std::max(s1.x, s2.x));
        int maxY = (int)std::max(s0.y, std::max(s1.y, s2.y));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float3 b = RenderUtils::BarycentricCoordinate(float2(x, y), s0.xy, s1.xy, s2.xy);
                if (b.x < RenderUtils::NegativeInfinity ||
                    b.y < RenderUtils::NegativeInfinity ||
                    b.z < RenderUtils::NegativeInfinity) continue;

                // Perspective-correct interpolation:
                // https://blog.csdn.net/Motarookie/article/details/124284471
                float z = 1.0f / (b.x / s0.w + b.y / s1.w + b.z / s2.w);

                float depth = b.x * s0.z + b.y * s1.z + b.z * s2.z;
                if (!gpu::DepthTest(depth, GetDepth(x, y))) continue;

                float b0 = b.x / s0.w * z;
                float b1 = b.y / s1.w * z;
                float b2 = b.z / s2.w * z;

                Varyings lerpV;
                InterpolateVaryings(*v0, *v1, *v2, b0, b1, b2, lerpV);

                float4 color = mat.fragment_shader(lerpV);

                RenderUtils::BlendMode blend = mat._data.transparent
                    ? RenderUtils::BlendMode::AlphaBlend
                    : RenderUtils::BlendMode::None;
                color = RenderUtils::BlendColors(color, GetColor(x, y), blend);
                SetColor(x, y, color);

                if (gpu::z_write && !mat._data.transparent)
                    SetDepth(x, y, depth);
            }
        }
    }

    void InterpolateVaryings(const Varyings& v0, const Varyings& v1, const Varyings& v2,
                             float b0, float b1, float b2, Varyings& out)
    {
        out.positionWS  = b0 * v0.positionWS  + b1 * v1.positionWS  + b2 * v2.positionWS;
        out.positionCS  = b0 * v0.positionCS  + b1 * v1.positionCS  + b2 * v2.positionCS;
        out.positionOS  = b0 * v0.positionOS  + b1 * v1.positionOS  + b2 * v2.positionOS;
        out.uv          = b0 * v0.uv          + b1 * v1.uv          + b2 * v2.uv;
        out.normalWS    = b0 * v0.normalWS    + b1 * v1.normalWS    + b2 * v2.normalWS;
        out.tangentWS   = b0 * v0.tangentWS   + b1 * v1.tangentWS   + b2 * v2.tangentWS;
        out.bitangentWS = b0 * v0.bitangentWS + b1 * v1.bitangentWS + b2 * v2.bitangentWS;
        out.fogFactor   = b0 * v0.fogFactor   + b1 * v1.fogFactor   + b2 * v2.fogFactor;
    }

    // Sutherland-Hodgman clipping against one plane
    std::vector<Varyings*> ClipWithPlane(RenderUtils::ClipPlane plane,
                                         std::vector<Varyings*> verts)
    {
        std::vector<Varyings*> out;
        int n = (int)verts.size();
        for (int i = 0; i < n; ++i) {
            Varyings* cur  = verts[i];
            Varyings* prev = verts[(i - 1 + n) % n];
            bool curIn  = RenderUtils::IsInsidePlane(plane, cur->positionCS);
            bool prevIn = RenderUtils::IsInsidePlane(plane, prev->positionCS);

            if (curIn != prevIn) {
                float t = RenderUtils::get_intersect_ratio(prev->positionCS, cur->positionCS, plane);
                Varyings* v = GetOneVaryings();
                v->positionCS = vec4_lerp(prev->positionCS, cur->positionCS, t);
                v->positionWS = vec3_lerp(prev->positionWS, cur->positionWS, t);
                v->normalWS   = vec4_lerp(prev->normalWS,   cur->normalWS,   t);
                v->uv         = vec2_lerp(prev->uv,         cur->uv,         t);
                out.push_back(v);
            }
            if (curIn) {
                Varyings* v = GetOneVaryings();
                *v = *cur;
                out.push_back(v);
            }
        }
        return out;
    }

    // Full triangle pipeline: vertex shader -> clip -> rasterize
    void DrawTriangle(const Vertex& vtx0, const Vertex& vtx1, const Vertex& vtx2,
                      const Material& mat)
    {
        Attributes* a0 = GetOneAttributes(); mat.InitAttributes(vtx0, a0);
        Attributes* a1 = GetOneAttributes(); mat.InitAttributes(vtx1, a1);
        Attributes* a2 = GetOneAttributes(); mat.InitAttributes(vtx2, a2);

        Varyings* v0 = GetOneVaryings(); mat.vertex_shader(a0, v0);
        Varyings* v1 = GetOneVaryings(); mat.vertex_shader(a1, v1);
        Varyings* v2 = GetOneVaryings(); mat.vertex_shader(a2, v2);

        if (RenderUtils::IsVisible(v0->positionCS) &&
            RenderUtils::IsVisible(v1->positionCS) &&
            RenderUtils::IsVisible(v2->positionCS))
        {
            DrawTriangle(v0, v1, v2, mat);
            return;
        }

        // Sutherland-Hodgman clipping against all 7 frustum planes
        std::vector<Varyings*> verts = { v0, v1, v2 };
        verts = ClipWithPlane(RenderUtils::ClipPlane::W_PLANE,  verts);
        verts = ClipWithPlane(RenderUtils::ClipPlane::X_RIGHT,  verts);
        verts = ClipWithPlane(RenderUtils::ClipPlane::X_LEFT,   verts);
        verts = ClipWithPlane(RenderUtils::ClipPlane::Y_TOP,    verts);
        verts = ClipWithPlane(RenderUtils::ClipPlane::Y_BOTTOM, verts);
        verts = ClipWithPlane(RenderUtils::ClipPlane::Z_NEAR,   verts);
        verts = ClipWithPlane(RenderUtils::ClipPlane::Z_FAR,    verts);

        // Fan-triangulate the clipped polygon
        for (int i = 0; i + 2 < (int)verts.size(); ++i)
            DrawTriangle(verts[0], verts[i + 1], verts[i + 2], mat);
    }
};
