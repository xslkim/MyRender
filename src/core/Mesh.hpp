#pragma once
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdint>
#include "mikktspace.h"
#include "Vertex.hpp"

class Mesh {
public:
    // All triangles, flat, concatenated in submesh order.
    std::vector<std::array<Vertex, 3>> triangles;

    // A submesh is a [start, count) range into `triangles`; one material binds
    // to one submesh. Legacy OBJ meshes have a single submesh spanning all faces.
    struct SubMesh { int start = 0; int count = 0; };
    std::vector<SubMesh> submeshes;

    Mesh(const std::string& fileName)
    {
        std::string ext = std::filesystem::path(fileName.c_str()).extension().string();
        if (ext == ".mesh")     loadBinary(fileName);
        else if (ext == ".obj") load(fileName);
        else                    assert(false && "unsupported mesh format");
    }

private:
    // -----------------------------------------------------------------------
    // Binary .mesh (MRSH) — Unity export, read verbatim (no x-flip, no winding
    // reversal). See docs/MyRender_AssetFormat.md.
    // -----------------------------------------------------------------------
    void loadBinary(const std::string& fileName)
    {
        std::ifstream in(fileName, std::ios::binary);
        if (in.fail()) { assert(false && "cannot open .mesh"); return; }

        auto u16 = [&] { uint16_t v; in.read((char*)&v, 2); return v; };
        auto u32 = [&] { uint32_t v; in.read((char*)&v, 4); return v; };
        auto f32 = [&] { float v;    in.read((char*)&v, 4); return v; };

        char magic[4];
        in.read(magic, 4);
        assert(std::string(magic, 4) == "MRSH" && "bad .mesh magic");

        uint16_t version = u16(); (void)version;
        uint16_t flags   = u16();
        bool hasSkin  = flags & (1 << 0);
        bool hasUV1   = flags & (1 << 1);
        bool hasColor = flags & (1 << 2);

        uint32_t vertexCount  = u32();
        uint32_t indexCount   = u32();
        uint32_t submeshCount = u32();
        uint32_t boneCount    = u32(); (void)boneCount;

        std::vector<std::pair<uint32_t, uint32_t>> ranges(submeshCount); // (indexStart, indexCount)
        for (auto& r : ranges) { r.first = u32(); r.second = u32(); }

        // Interleaved vertices. uv1/color/skin are parsed-and-skipped for now
        // (the shader path doesn't consume them yet) — kept readable, not dropped silently.
        std::vector<Vertex> verts;
        verts.reserve(vertexCount);
        for (uint32_t i = 0; i < vertexCount; ++i) {
            Vec3f pos(f32(), f32(), f32());
            Vec3f nrm(f32(), f32(), f32());
            Vec4f tan(f32(), f32(), f32(), f32());
            Vec2f uv0(f32(), f32());
            if (hasUV1)   { f32(); f32(); }
            if (hasColor) { f32(); f32(); f32(); f32(); }
            if (hasSkin)  { u16(); u16(); u16(); u16(); f32(); f32(); f32(); f32(); }
            Vertex v(pos, uv0, nrm);
            v.tangent = tan;
            verts.push_back(v);
        }

        std::vector<uint32_t> indices(indexCount);
        for (auto& idx : indices) idx = u32();

        // Build flat triangles per submesh, recording each submesh's range.
        for (const auto& r : ranges) {
            SubMesh sm;
            sm.start = (int)triangles.size();
            for (uint32_t k = 0; k + 2 < r.second; k += 3) {
                uint32_t a = indices[r.first + k];
                uint32_t b = indices[r.first + k + 1];
                uint32_t c = indices[r.first + k + 2];
                triangles.push_back({ verts[a], verts[b], verts[c] });
            }
            sm.count = (int)triangles.size() - sm.start;
            submeshes.push_back(sm);
        }
    }

    struct FaceIndex
    {
        FaceIndex(int p, int uv, int n) : i_pos(p), i_uv(uv), i_normal(n) {}
        int i_pos, i_uv, i_normal;
    };

    void load(const std::string& fileName)
    {
        std::vector<Vec3f> positions;
        std::vector<Vec2f> texcoords;
        std::vector<Vec3f> normals;

        std::ifstream in(fileName);
        if (in.fail()) return;

        std::string line;
        while (!in.eof()) {
            std::getline(in, line);
            std::istringstream iss(line.c_str());
            char trash;

            if (line.compare(0, 2, "v ") == 0) {
                iss >> trash;
                Vec3f pos;
                for (int i = 0; i < 3; i++) iss >> pos[i];
                pos.x = -pos.x;
                positions.push_back(pos);
            }
            else if (line.compare(0, 3, "vn ") == 0) {
                iss >> trash >> trash;
                Vec3f n;
                for (int i = 0; i < 3; i++) iss >> n[i];
                n.x = -n.x;
                normals.push_back(n);
            }
            else if (line.compare(0, 3, "vt ") == 0) {
                iss >> trash >> trash;
                Vec2f uv;
                iss >> uv[0] >> uv[1];
                texcoords.push_back(uv);
            }
            else if (line.compare(0, 2, "f ") == 0) {
                std::vector<FaceIndex> fs;
                int i_pos, i_uv, i_normal;
                iss >> trash;
                while (iss >> i_pos >> trash >> i_uv >> trash >> i_normal) {
                    fs.push_back(FaceIndex(i_pos - 1, i_uv - 1, i_normal - 1));
                }

                auto makeVertex = [&](const FaceIndex& f) {
                    return Vertex(positions[f.i_pos], texcoords[f.i_uv], normals[f.i_normal]);
                };

                if (fs.size() == 3) {
                    // winding order: reverse vertices 1 and 2 to match coordinate system
                    triangles.push_back({ makeVertex(fs[0]), makeVertex(fs[2]), makeVertex(fs[1]) });
                }
                else if (fs.size() == 4) {
                    triangles.push_back({ makeVertex(fs[0]), makeVertex(fs[1]), makeVertex(fs[2]) });
                    triangles.push_back({ makeVertex(fs[0]), makeVertex(fs[2]), makeVertex(fs[3]) });
                }
                else {
                    assert(false && "only triangles and quads are supported");
                }
            }
        }

        CalcTangents();

        // Legacy OBJ = one submesh spanning every face.
        submeshes.push_back(SubMesh{ 0, (int)triangles.size() });
    }

    // MikkTSpace tangent calculation
    void CalcTangents()
    {
        SMikkTSpaceInterface iface{};
        iface.m_getNumFaces            = [](const SMikkTSpaceContext* ctx) { return (int)static_cast<Mesh*>(ctx->m_pUserData)->triangles.size(); };
        iface.m_getNumVerticesOfFace   = [](const SMikkTSpaceContext*, int) { return 3; };
        iface.m_getPosition            = [](const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
            auto& p = static_cast<Mesh*>(ctx->m_pUserData)->triangles[face][vert].position;
            out[0] = p.x; out[1] = p.y; out[2] = p.z;
        };
        iface.m_getNormal              = [](const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
            auto& n = static_cast<Mesh*>(ctx->m_pUserData)->triangles[face][vert].normal;
            out[0] = n.x; out[1] = n.y; out[2] = n.z;
        };
        iface.m_getTexCoord            = [](const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
            auto& uv = static_cast<Mesh*>(ctx->m_pUserData)->triangles[face][vert].texcoord;
            out[0] = uv.x; out[1] = uv.y;
        };
        iface.m_setTSpaceBasic         = [](const SMikkTSpaceContext* ctx, const float tan[], float sign, int face, int vert) {
            auto& t = static_cast<Mesh*>(ctx->m_pUserData)->triangles[face][vert].tangent;
            t.x = tan[0]; t.y = tan[1]; t.z = tan[2]; t.w = sign;
        };

        SMikkTSpaceContext context{};
        context.m_pInterface = &iface;
        context.m_pUserData  = this;
        genTangSpaceDefault(&context);
    }
};
