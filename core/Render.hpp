#pragma once
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

private:
    float* ColorBuffer;
    float* DepthBuffer;
    const int kColorBufferSize = Config::kScreenWidth * Config::kScreenHeight * 4;
    const int kDepthBufferSize = Config::kScreenWidth * Config::kScreenHeight;
    float* default_color_buf;
    float* default_depth_buf;
    Vec4f default_color = { 0, 0, 0, 1 };
    float default_depth = 1;
public:
    
    void Init() {
        

        ColorBuffer = new float[kColorBufferSize];
        DepthBuffer = new float[kDepthBufferSize];
        default_color_buf = new float[kColorBufferSize];
        for (int x = 0; x < Config::kScreenWidth; x++)
        {
            for (int y = 0; y < Config::kScreenHeight; ++y)
            {
                int i = y * Config::kScreenWidth + x;
                i *= 4;
                default_color_buf[i + 0] = default_color.r;
                default_color_buf[i + 1] = default_color.g;
                default_color_buf[i + 2] = default_color.b;
                default_color_buf[i + 3] = default_color.w;
            }
        }
        default_depth_buf = new float[kDepthBufferSize];
        for (int i = 0; i < kDepthBufferSize; i += 1)
        {
            default_depth_buf[i] = default_depth;
        }
        ClearColorBuffer();
        ClearDepthBuffer();
    }

    void ClearColorBuffer()
    {
        memcpy(ColorBuffer, default_color_buf, kColorBufferSize*sizeof(float));
    }

    void ClearDepthBuffer()
    {
        memcpy(DepthBuffer, default_depth_buf, kDepthBufferSize * sizeof(float));
    }

    void FrameStart(const Camera& cam, const Light& light) 
    {
        ResetProgramBuffer();
        Render::Get().ClearColorBuffer();
        Render::Get().ClearDepthBuffer();
        gpu::_ProjectionParams.x = 1;
        gpu::_ProjectionParams.y = cam.Near;
        gpu::_ProjectionParams.z = cam.Far;
        gpu::_ProjectionParams.w = 1.f/cam.Far;

        gpu::_WorldSpaceCameraPos = cam.Position;
        gpu::_ScaledScreenParams.x = Config::kScreenWidth;
        gpu::_ScaledScreenParams.y = Config::kScreenWidth;

        float3 light_dir = RenderUtils::RotationToDirection(light.Rotation.x, light.Rotation.y, light.Rotation.z, Vec3f(0, 0, 1));
        gpu::_MainLightPosition.xyz = light_dir;
        gpu::_MainLightColor = light.Color;

    }

    float GetDepth(int x, int y, int sampleIndex = 0)
    {
        float depth = DepthBuffer[Config::kScreenWidth * y + x];
        return depth;
    }

    void SetDepth(int x, int y, float depth)
    {
        DepthBuffer[Config::kScreenWidth * y + x] = depth;
    }

    inline float4 GetColor(int x, int y, int sampleIndex = 0)
    {
        float4 color;
        int index = y * Config::kScreenWidth + x;
        index *= 4;
        color.r = ColorBuffer[index];
        color.g = ColorBuffer[index + 1];
        color.b = ColorBuffer[index + 2];
        color.a = ColorBuffer[index + 3];
        return color;
    }

    inline void SetColor(int x, int y, float4 color)
    {
        int index = y * Config::kScreenWidth + x;
        index *= 4;
        ColorBuffer[index] = color.r;
        ColorBuffer[index + 1] = color.g;
        ColorBuffer[index + 2] = color.b;
        ColorBuffer[index + 3] = color.a;
    }


    void UpdateModelMatrix(GameObject& obj)
    {
        float4x4 translateMatrix = createMatrix4x4<float>
            (   1, 0, 0, obj.Position.x,
                0, 1, 0, obj.Position.y,
                0, 0, 1, obj.Position.z,
                0, 0, 0, 1);

        float4x4 scaleMatrix = createMatrix4x4<float>
            (   obj.Scale.x, 0, 0, 0,
                0, obj.Scale.y, 0, 0,
                0, 0, obj.Scale.z, 0,
                0, 0, 0, 1);

        float rx = obj.Rotation.x * PI / 180.f;
        float4x4 rotateX = createMatrix4x4<float>
            (   1, 0, 0, 0,
                0, cos(rx), -sin(rx), 0,
                0, sin(rx), cos(rx), 0,
                0, 0, 0, 1);

        float ry = obj.Rotation.y * PI / 180.f;
        float4x4 rotateY = createMatrix4x4<float>
            (   cos(ry), 0, sin(ry), 0,
                0, 1, 0, 0,
                -sin(ry), 0, cos(ry), 0,
                0, 0, 0, 1);

        float rz = obj.Rotation.z * PI / 180.f;
        //注意，这的旋转方向和view的旋转方向不一致
        float4x4 rotateZ = createMatrix4x4<float>
            (   cos(rz), -sin(rz), 0, 0,
                sin(rz), cos(rz), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
        float4x4 rotateMatrix = rotateY * rotateX * rotateZ;
        float4x4 rsMat = rotateMatrix * scaleMatrix;
        float4x4 i_rsMat = rsMat.Transpose();
        float4x4 m = translateMatrix * rotateMatrix * scaleMatrix;
        gpu::UNITY_MATRIX_M = m;
        float4x4 i_translateMatrix = createMatrix4x4<float>
            (1, 0, 0, -obj.Position.x,
                0, 1, 0, -obj.Position.y,
                0, 0, 1, -obj.Position.z,
                0, 0, 0, 1);
        float4x4 i_m = i_translateMatrix * i_rsMat;
        gpu::UNITY_MATRIX_I_M = i_m;

    }

    void UpdateViewMatrix(const Camera& camera)
    {
        float4x4 init_v = createMatrix4x4<float>(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, -1, 0,
            0, 0, 0, 1);

        float rx = camera.Rotation.x * PI / 180.f;
        float4x4 rotateX = createMatrix4x4<float>
            (1, 0, 0, 0,
                0, cos(rx), -sin(rx), 0,
                0, sin(rx), cos(rx), 0,
                0, 0, 0, 1);

        float ry = camera.Rotation.y * PI / 180.f;
        float4x4 rotateY = createMatrix4x4<float>
            (cos(ry), 0, sin(ry), 0,
                0, 1, 0, 0,
                -sin(ry), 0, cos(ry), 0,
                0, 0, 0, 1);

        float rz = camera.Rotation.z * PI / 180.f;
        float4x4 rotateZ = createMatrix4x4<float>
            (cos(rz), sin(rz), 0, 0,
                -sin(rz), cos(rz), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);

        float4x4 rotate = rotateZ * rotateX * rotateY;
        float4x4 rv =  rotate * init_v;

        float4x4 m = createMatrix4x4<float>(
            1, 0, 0, -camera.Position.x,
            0, 1, 0, -camera.Position.y,
            0, 0, 1, -camera.Position.z,
            0, 0, 0, 1);
        float4x4 v = rv * m;

        gpu::UNITY_MATRIX_V = v;
    }

    void UpdateProjectMatrix(const Camera& camera)
    {
        float rad = camera.Fov * PI / 180.f;
        float tanHalfFov = tan(rad / 2);
        float fovY = 1 / tanHalfFov;
        float fovX = fovY / camera.Aspect;
        float4x4 p = createMatrix4x4<float>(
            fovX, 0, 0, 0,
            0, fovY, 0, 0,
            0, 0, -(camera.Far + camera.Near) / (camera.Far - camera.Near), -(2 * camera.Far * camera.Near) / (camera.Far - camera.Near),
            0, 0, -1, 0
        );
        gpu::UNITY_MATRIX_P = p;
        gpu::UNITY_MATRIX_VP = p * gpu::UNITY_MATRIX_V;
    }

    bool IsFrontFace(float3 v0, float3 v1, float3 v2)
    {
        float3 e1 = v1 - v0;
        float3 e2 = v2 - v0;
        float3 normal = vector_cross(e1, e2);
        return normal.z < 0;
    }

    void DrawTriangle(const Varyings* v0, const Varyings* v1, const Varyings* v2, const Material& mat)
    {
        float3 ndcPos0;
        float3 ndcPos1;
        float3 ndcPos2;
        float4 screen0 = RenderUtils::ClipPositionToScreenPosition(v0->positionCS, ndcPos0);
        float4 screen1 = RenderUtils::ClipPositionToScreenPosition(v1->positionCS, ndcPos1);
        float4 screen2 = RenderUtils::ClipPositionToScreenPosition(v2->positionCS, ndcPos2);
        if (!IsFrontFace(ndcPos0, ndcPos1, ndcPos2))
        {
            return;
        }
            

        // 计算三角形的边界框
        int bboxMinX = min(screen0.x, min(screen1.x, screen2.x));
        int bboxMinY = min(screen0.y, min(screen1.y, screen2.y));


        int bboxMaxX = max(screen0.x, max(screen1.x, screen2.x));
        int bboxMaxY = max(screen0.y, max(screen1.y, screen2.y));

        // 遍历边界框内的每个像素
        for (int y = bboxMinY; y <= bboxMaxY; y++)
        {
            for (int x = bboxMinX; x <= bboxMaxX; x++)
            {
                // 计算像素的重心坐标
                float3 bout = RenderUtils::BarycentricCoordinate(float2(x, y), screen0.xy, screen1.xy, screen2.xy);
                float barycenter0 = bout.x;
                float barycenter1 = bout.y;
                float barycenter2 = bout.z;
                // 如果像素在三角形内，则绘制该像素(NegativeInfinity避免误差)
                if (barycenter0 >= RenderUtils::NegativeInfinity && barycenter1 >= RenderUtils::NegativeInfinity && barycenter2 >= RenderUtils::NegativeInfinity)
                {
                    /// ================================ 透视矫正 ================================
                    // 推导公式:https://blog.csdn.net/Motarookie/article/details/124284471
                    // z是当前像素在摄像机空间中的深度值。
                    // 插值校正系数
                    float z = 1 / (barycenter0 / screen0.w + barycenter1 / screen1.w + barycenter2 / screen2.w);


                    /// ================================ 当前像素的深度插值 ================================
                    float depth = barycenter0 * screen0.z + barycenter1 * screen1.z + barycenter2 * screen2.z;

                    auto depthBuffer = GetDepth(x, y);
                    // 深度测试
                    if (gpu::DepthTest(depth, depthBuffer))
                    {
                        // 进行透视矫正
                        barycenter0 = barycenter0 / screen0.w * z;
                        barycenter1 = barycenter1 / screen1.w * z;
                        barycenter2 = barycenter2 / screen2.w * z;

                        // 插值顶点属性
                        Varyings lerpVertex;
                        InterpolateVertexOutputs(*v0, *v1, *v2, barycenter0, barycenter1, barycenter2, lerpVertex);
                        float4 color = mat.fragment_shader(lerpVertex);
                        RenderUtils::BlendMode blendMode = RenderUtils::BlendMode::None;
                        if (mat._data.transparent)
                        {
                            blendMode = RenderUtils::BlendMode::AlphaBlend;
                        }
                        
                        color = RenderUtils::BlendColors(color, GetColor(x, y), blendMode);
                        SetColor(x, y, color);
                        if (gpu::z_write && !mat._data.transparent)
                        {
                            SetDepth(x, y, depth);
                        }
                        
                    }
                }
            }
        }

    }

    void InterpolateVertexOutputs(const Varyings& v0, const Varyings& v1, const Varyings& v2, float barycenter0, float barycenter1, float barycenter2, Varyings& result)
    {
        result.positionWS = barycenter0 * v0.positionWS + barycenter1 * v1.positionWS + barycenter2 * v2.positionWS;
        result.positionCS = barycenter0 * v0.positionCS + barycenter1 * v1.positionCS + barycenter2 * v2.positionCS;
        result.positionOS = barycenter0 * v0.positionOS + barycenter1 * v1.positionOS + barycenter2 * v2.positionOS;
        result.uv = barycenter0 * v0.uv + barycenter1 * v1.uv + barycenter2 * v2.uv;
        result.normalWS = barycenter0 * v0.normalWS + barycenter1 * v1.normalWS + barycenter2 * v2.normalWS;
        result.tangentWS = barycenter0 * v0.tangentWS + barycenter1 * v1.tangentWS + barycenter2 * v2.tangentWS;
        result.bitangentWS = barycenter0 * v0.bitangentWS + barycenter1 * v1.bitangentWS + barycenter2 * v2.bitangentWS;
        result.fogFactor = barycenter0 * v0.fogFactor + barycenter1 * v1.fogFactor + barycenter2 * v2.fogFactor;
    }


    void DrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex v2, const Material& mat)
    {
        Attributes* attributes0 = GetOneAttributes();
        Attributes* attributes1 = GetOneAttributes();
        Attributes* attributes2 = GetOneAttributes();
        mat.InitAttributes(v0, attributes0);
        mat.InitAttributes(v1, attributes1);
        mat.InitAttributes(v2, attributes2);


        Varyings* varyings0 = GetOneVaryings();
        Varyings* varyings1 = GetOneVaryings();
        Varyings* varyings2 = GetOneVaryings();

        mat.vertex_shader(attributes0, varyings0);
        mat.vertex_shader(attributes1, varyings1);
        mat.vertex_shader(attributes2, varyings2);

        if (RenderUtils::IsVisible(varyings0->positionCS) && RenderUtils::IsVisible(varyings1->positionCS) && RenderUtils::IsVisible(varyings2->positionCS))
        {
            DrawTriangle(varyings0, varyings1, varyings2, mat);
        }
        else 
        {
            std::vector<Varyings*> TriangleVaryings;
            TriangleVaryings.push_back(varyings0);
            TriangleVaryings.push_back(varyings1);
            TriangleVaryings.push_back(varyings2);
            TriangleVaryings = ClipWithPlane(RenderUtils::ClipPlane::W_PLANE, TriangleVaryings.size(), TriangleVaryings);

            TriangleVaryings = ClipWithPlane(RenderUtils::ClipPlane::X_RIGHT, TriangleVaryings.size(), TriangleVaryings);
            TriangleVaryings = ClipWithPlane(RenderUtils::ClipPlane::X_LEFT, TriangleVaryings.size(), TriangleVaryings);
            TriangleVaryings = ClipWithPlane(RenderUtils::ClipPlane::Y_TOP, TriangleVaryings.size(), TriangleVaryings);
            TriangleVaryings = ClipWithPlane(RenderUtils::ClipPlane::Y_BOTTOM, TriangleVaryings.size(), TriangleVaryings);
            TriangleVaryings = ClipWithPlane(RenderUtils::ClipPlane::Z_NEAR, TriangleVaryings.size(), TriangleVaryings);
            TriangleVaryings = ClipWithPlane(RenderUtils::ClipPlane::Z_FAR, TriangleVaryings.size(), TriangleVaryings);
            if (TriangleVaryings.size() >= 3)
            {
                for (int i = 0; i < TriangleVaryings.size() - 2; i++)
                {
                    int index0 = 0;
                    int index1 = i + 1;
                    int index2 = i + 2;
                    DrawTriangle(TriangleVaryings[index0], TriangleVaryings[index1], TriangleVaryings[index2], mat);
                }
            }
        }

        

        

    }

    std::vector<Varyings*> ClipWithPlane(RenderUtils::ClipPlane c_plane, int num_vertex, std::vector<Varyings*> triangle_varyings)
    {
        std::vector<Varyings*> out_varyings;
        int num_vert = triangle_varyings.size();
        for (int i = 0; i < num_vert; ++i)
        {
            int current_index = i;
            int previous_index = (i - 1 + triangle_varyings.size()) % num_vert;
            Varyings* cur_varying = triangle_varyings[current_index];
            Varyings* pre_varying = triangle_varyings[previous_index];

            bool is_cur_inside = RenderUtils::IsInsidePlane(c_plane, cur_varying->positionCS);
            bool is_pre_inside = RenderUtils::IsInsidePlane(c_plane, pre_varying->positionCS);

            if (is_cur_inside != is_pre_inside)
            {
                float t = RenderUtils::get_intersect_ratio(pre_varying->positionCS, cur_varying->positionCS, c_plane);
                Varyings* one_varying = GetOneVaryings();
                one_varying->positionCS = vec4_lerp(pre_varying->positionCS, cur_varying->positionCS, t);
                one_varying->positionWS = vec3_lerp(pre_varying->positionWS, cur_varying->positionWS, t);
                one_varying->normalWS = vec4_lerp(pre_varying->normalWS, cur_varying->normalWS, t);
                one_varying->uv = vec2_lerp(pre_varying->uv, cur_varying->uv, t);
                out_varyings.push_back(one_varying);
            }

            if (is_cur_inside == true)
            {
                Varyings* one_varying = GetOneVaryings();
                *one_varying = *cur_varying;
                out_varyings.push_back(one_varying);
            }

        }

        return out_varyings;
    }


    void Draw(const Camera& cam, const Mesh& mesh, const Material& mat)
    {
        mat.UpdateGpuParameter();
        
        for (int i = 0; i < mesh.triangles.size(); ++i)
        {
            bool runFragShader = true;
            Varyings* triangleVarying[3];
            const  Vertex& v0 = mesh.triangles[i][0];
            const  Vertex& v1 = mesh.triangles[i][1];
            const  Vertex& v2 = mesh.triangles[i][2];
            DrawTriangle(v0, v1, v2, mat);
        }
    }
};
