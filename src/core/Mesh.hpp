#pragma once
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "mikktspace.h"
#include "Vertex.hpp"

class Mesh {
public:
    std::vector<std::array<Vertex, 3>> triangles;

    Mesh(const std::string& fileName)
    {
        std::string ext = std::filesystem::path(fileName.c_str()).extension().string();
        assert(ext == ".obj");
        load(fileName);
    }

private:
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
