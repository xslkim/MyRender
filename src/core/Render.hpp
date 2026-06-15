#pragma once
#include <vector>
#include <array>
#include <future>
#include <thread>
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
#include "SceneModel.hpp"

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

        _numThreads = std::max(1u, std::thread::hardware_concurrency());
        _pendingTris.reserve(100000);
    }

    // Override rasterizer thread count (used for the single-vs-multi-thread demo).
    void SetNumThreads(unsigned n) { _numThreads = std::max(1u, n); }
    unsigned GetNumThreads() const { return _numThreads; }
    unsigned GetHardwareThreads() const { return std::max(1u, std::thread::hardware_concurrency()); }

    // -------------------------------------------------------------------------
    // Per-frame setup — the renderer consumes matrices only (SceneModel).
    // Front-end loaders own all TRS/euler/handedness concerns.
    // -------------------------------------------------------------------------

    void BeginFrame()
    {
        ResetFrameBuffers();
        ClearColorBuffer();
        ClearDepthBuffer();
    }

    void SetCamera(const CameraState& cam)
    {
        gpu::UNITY_MATRIX_V  = cam.view;
        gpu::UNITY_MATRIX_P  = cam.projection;
        gpu::UNITY_MATRIX_VP = cam.projection * cam.view;

        gpu::_WorldSpaceCameraPos  = cam.position;
        gpu::_ProjectionParams.x   = 1;
        gpu::_ProjectionParams.y   = cam.near;
        gpu::_ProjectionParams.z   = cam.far;
        gpu::_ProjectionParams.w   = 1.0f / cam.far;
        gpu::_ScaledScreenParams.x = Config::kScreenWidth;
        gpu::_ScaledScreenParams.y = Config::kScreenHeight;
    }

    void SetLight(const LightState& light)
    {
        // URP convention: _MainLightPosition.xyz points *toward* the light.
        gpu::_MainLightPosition.xyz = -light.direction;
        gpu::_MainLightColor        = light.color;
    }

    void SetModelMatrices(const float4x4& localToWorld, const float4x4& worldToLocal)
    {
        gpu::UNITY_MATRIX_M   = localToWorld;
        gpu::UNITY_MATRIX_I_M = worldToLocal;
    }

    // +1 = legacy OBJ winding (x-mirrored, reversed). -1 = verbatim Unity winding.
    void SetFrontFaceSign(float sign) { _frontFaceSign = sign; }

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
    // Draw call — two-pass: vertex/clip (single-threaded) then rasterize (parallel)
    //
    // Why two passes?
    //   Vertex shading WRITES gpu:: namespace globals (matrices, textures, etc.).
    //   Fragment shading only READS those globals.
    //   So we can safely rasterize on multiple threads once vertex shading is done.
    // -------------------------------------------------------------------------

    // Draw one submesh with one material. Callers iterate an object's submeshes;
    // model matrices are uploaded once per object via SetModelMatrices.
    void Draw(const Mesh& mesh, const Mesh::SubMesh& sub, const Material& mat)
    {
        mat.UpdateGpuParameter();

        // Pass 1 (single-threaded): vertex shader + Sutherland-Hodgman clip
        _pendingTris.clear();
        int end = sub.start + sub.count;
        for (int i = sub.start; i < end; ++i) {
            const auto& tri = mesh.triangles[i];
            CollectTriangle(tri[0], tri[1], tri[2], mat);
        }

        if (_pendingTris.empty()) return;

        // Pass 2 (multi-threaded): rasterize by horizontal Y strip.
        // Each thread owns a distinct Y range -> no pixel is written by two threads.
        // Fragment shaders only read gpu:: globals -> no data race.
        int rowsPerThread = (Config::kScreenHeight + (int)_numThreads - 1) / (int)_numThreads;

        // Expose the strip layout so DV_THREADS can tint each thread's band.
        gpu::g_dbgRowsPerStrip = rowsPerThread;
        gpu::g_dbgThreadCount  = (int)_numThreads;

        std::vector<std::future<void>> futures;
        futures.reserve(_numThreads);

        for (unsigned t = 0; t < _numThreads; ++t) {
            int yStart = (int)t * rowsPerThread;
            int yEnd   = std::min(yStart + rowsPerThread, Config::kScreenHeight);
            if (yStart >= Config::kScreenHeight) break;

            futures.push_back(std::async(std::launch::async,
                [this, &mat, yStart, yEnd]() {
                    for (const auto& tri : _pendingTris)
                        RasterizeTri(tri[0], tri[1], tri[2], mat, yStart, yEnd);
                }));
        }

        for (auto& f : futures) f.get();
    }

private:
    // -------------------------------------------------------------------------
    // Per-frame allocator for Attributes / Varyings (used in single-threaded pass)
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
    // Thread count (set at Init time)
    // -------------------------------------------------------------------------
    unsigned _numThreads = 1;

    // Winding convention for front-face test (see IsFrontFace / SetFrontFaceSign).
    float _frontFaceSign = 1.0f;

    // Screen-space triangles collected during the single-threaded vertex pass.
    // Stored as value types so threads can safely read them in parallel.
    using ScreenTri = std::array<Varyings, 3>;
    std::vector<ScreenTri> _pendingTris;

    // -------------------------------------------------------------------------
    // Geometry helpers
    // -------------------------------------------------------------------------

    // Front-face test in screen space. The sign depends on the data's winding
    // convention: the legacy OBJ path (x-mirror + winding reversal) wants one
    // sign, verbatim Unity data the other. Scene picks via SetFrontFaceSign.
    bool IsFrontFace(float3 v0, float3 v1, float3 v2)
    {
        return vector_cross(v1 - v0, v2 - v0).z * _frontFaceSign < 0;
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

    // Sutherland-Hodgman: clip a polygon against one frustum plane
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

    // -------------------------------------------------------------------------
    // Pass 1: vertex shader + clip -> push screen-space triangles to _pendingTris
    // Called single-threaded (may write gpu:: globals via vertex shader).
    // -------------------------------------------------------------------------
    void CollectTriangle(const Vertex& vtx0, const Vertex& vtx1, const Vertex& vtx2,
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
            _pendingTris.push_back({ *v0, *v1, *v2 });
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

        // Fan-triangulate and store copies (threads will read these)
        for (int i = 0; i + 2 < (int)verts.size(); ++i)
            _pendingTris.push_back({ *verts[0], *verts[i + 1], *verts[i + 2] });
    }

    // -------------------------------------------------------------------------
    // Pass 2: rasterize one screen-space triangle within Y strip [yStart, yEnd).
    // Called from multiple threads simultaneously — must not write gpu:: globals.
    // -------------------------------------------------------------------------
    void RasterizeTri(const Varyings& v0, const Varyings& v1, const Varyings& v2,
                      const Material& mat, int yStart, int yEnd)
    {
        float3 ndc0, ndc1, ndc2;
        float4 s0 = RenderUtils::ClipPositionToScreenPosition(v0.positionCS, ndc0);
        float4 s1 = RenderUtils::ClipPositionToScreenPosition(v1.positionCS, ndc1);
        float4 s2 = RenderUtils::ClipPositionToScreenPosition(v2.positionCS, ndc2);

        // Cull per material. Front-face winding sign for verbatim Unity data is
        // resolved empirically at M1 (T1.5); IsFrontFace defines "front".
        bool front = IsFrontFace(ndc0, ndc1, ndc2);
        switch (mat.cull) {
            case Material::Cull::Back:  if (!front) return; break;
            case Material::Cull::Front: if ( front) return; break;
            case Material::Cull::Off:                        break;
        }

        // Bounding box, clamped to screen and this thread's Y strip
        int minX = std::max(0,                       (int)std::min(s0.x, std::min(s1.x, s2.x)));
        int maxX = std::min(Config::kScreenWidth - 1, (int)std::max(s0.x, std::max(s1.x, s2.x)));
        int minY = std::max(yStart,                   (int)std::min(s0.y, std::min(s1.y, s2.y)));
        int maxY = std::min(yEnd - 1,                 (int)std::max(s0.y, std::max(s1.y, s2.y)));

        if (minX > maxX || minY > maxY) return;

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float3 b = RenderUtils::BarycentricCoordinate(float2(x, y), s0.xy, s1.xy, s2.xy);
                if (b.x < RenderUtils::NegativeInfinity ||
                    b.y < RenderUtils::NegativeInfinity ||
                    b.z < RenderUtils::NegativeInfinity) continue;

                // Perspective-correct interpolation
                // https://blog.csdn.net/Motarookie/article/details/124284471
                float z = 1.0f / (b.x / s0.w + b.y / s1.w + b.z / s2.w);

                float depth = b.x * s0.z + b.y * s1.z + b.z * s2.z;
                if (!gpu::DepthTest(depth, GetDepth(x, y))) continue;

                // Depth-tested wireframe: edge pixels bright, interior dark fill.
                if (gpu::g_debugView == gpu::DV_WIRE) {
                    float m = std::min(b.x, std::min(b.y, b.z));
                    float4 c = (m < 0.018f) ? float4(0.40f, 0.85f, 1.0f, 1)
                                            : float4(0.05f, 0.06f, 0.09f, 1);
                    SetColor(x, y, c);
                    SetDepth(x, y, depth);
                    continue;
                }

                float b0 = b.x / s0.w * z;
                float b1 = b.y / s1.w * z;
                float b2 = b.z / s2.w * z;

                Varyings lerpV;
                InterpolateVaryings(v0, v1, v2, b0, b1, b2, lerpV);

                float4 color = mat.fragment_shader(lerpV);

                RenderUtils::BlendMode blend = mat._data.transparent
                    ? RenderUtils::BlendMode::AlphaBlend
                    : RenderUtils::BlendMode::None;
                color = RenderUtils::BlendColors(color, GetColor(x, y), blend);

                // Tint each rasterizer thread's Y-strip a distinct color.
                if (gpu::g_debugView == gpu::DV_THREADS) {
                    int    strip = y / std::max(1, gpu::g_dbgRowsPerStrip);
                    float4 band  = gpu::DebugStripColor(strip);
                    color = float4(color.r * 0.55f + band.r * 0.45f,
                                   color.g * 0.55f + band.g * 0.45f,
                                   color.b * 0.55f + band.b * 0.45f,
                                   color.a);
                }
                SetColor(x, y, color);

                if (gpu::z_write && !mat._data.transparent)
                    SetDepth(x, y, depth);
            }
        }
    }
};
