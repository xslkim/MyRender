#pragma once
#include "mikktspace.h"
#include "Vertex.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>


using namespace std;
class Mesh {
public:
    Mesh(const string& fileName)
    {
        string extension = filesystem::path(fileName.c_str()).extension().string();
        if (extension == ".obj") {
            load(fileName);
        }
        else {
            assert(false);
        }
    }

    vector<vector<Vertex>> triangles;

    struct FaceIndex
    {
        FaceIndex(int p, int uv, int n) :i_pos(p), i_uv(uv),i_normal(n){}
        int i_pos;
        int i_uv;
        int i_normal;
    };

    //struct VertexBuf
    //{
    //    Vec3f position;
    //    Vec4f tangent;
    //};

    //struct LoadBuffer {
        //vector<Vec3f> positions;
        //vector<Vec2f> texcoords;
        //vector<Vec3f> normals;
        //vector<vector<int>> triangles;
    //    map<string, int> index_map;
    //};


    void load(const string& fileName)
    {
        //LoadBuffer load_buf;

        vector<Vec3f> positions;
        vector<Vec2f> texcoords;
        vector<Vec3f> normals;

        std::ifstream in;
        in.open(fileName, std::ifstream::in);
        if (in.fail()) return;
        std::string line;
        while (!in.eof()) {
            std::getline(in, line);
            std::istringstream iss(line.c_str());
            char trash;
            if (line.compare(0, 2, "v ") == 0) {
                iss >> trash;
                Vec3f pos;
                for (int i = 0; i < 3; i++) 
                    iss >> pos[i];
                pos.x = -pos.x;
                positions.push_back(pos);
            }
            else if (line.compare(0, 3, "vn ") == 0) {
                iss >> trash >> trash;
                Vec3f n;
                for (int i = 0; i < 3; i++) 
                    iss >> n[i];
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
                int i_pos;
                int i_uv;
                int i_normal;
                iss >> trash;
                while (iss >> i_pos >> trash >> i_uv >> trash >> i_normal) 
                {
                    i_pos -= 1;
                    i_uv -= 1;
                    i_normal -= 1;
                    FaceIndex f(i_pos, i_uv, i_normal);
                    /*int i_vec;
                    if (load_buf.index_map.contains(key))
                    {
                        i_vec = load_buf.index_map[key];
                    }
                    else
                    {
                        i_vec = load_buf.vertexBuf.size();
                        VertexBuf vb;
                        vb.position = positions[i_pos];
                        load_buf.vertexBuf.push_back(vb);
                        load_buf.normals.push_back(normals[i_normal]);
                        load_buf.texcoords.push_back(texcoords[i_uv]);
                        load_buf.index_map[key] = i_vec;
                    }
                    */
                    fs.push_back(f);
                }

                if (fs.size() == 3)
                {
                    vector<Vertex> triangle;
                    int i0 = 0;
                    int i1 = 2;
                    int i2 = 1;
                    triangle.push_back(Vertex(positions[fs[i0].i_pos], texcoords[fs[i0].i_uv], normals[fs[i0].i_normal]));
                    triangle.push_back(Vertex(positions[fs[i1].i_pos], texcoords[fs[i1].i_uv], normals[fs[i1].i_normal]));
                    triangle.push_back(Vertex(positions[fs[i2].i_pos], texcoords[fs[i2].i_uv], normals[fs[i2].i_normal]));
                    triangles.push_back(triangle);
                }
                else if (fs.size() == 4)
                {
                    vector<Vertex> triangle1;
                    triangle1.push_back(Vertex(positions[fs[0].i_pos], texcoords[fs[0].i_uv], normals[fs[0].i_normal]));
                    triangle1.push_back(Vertex(positions[fs[1].i_pos], texcoords[fs[1].i_uv], normals[fs[1].i_normal]));
                    triangle1.push_back(Vertex(positions[fs[2].i_pos], texcoords[fs[2].i_uv], normals[fs[2].i_normal]));
                    triangles.push_back(triangle1);
                    vector<Vertex> triangle2;
                    triangle2.push_back(Vertex(positions[fs[0].i_pos], texcoords[fs[0].i_uv], normals[fs[0].i_normal]));
                    triangle2.push_back(Vertex(positions[fs[2].i_pos], texcoords[fs[2].i_uv], normals[fs[2].i_normal]));
                    triangle2.push_back(Vertex(positions[fs[3].i_pos], texcoords[fs[3].i_uv], normals[fs[3].i_normal]));
                    triangles.push_back(triangle2);
                }
                else
                {
                    assert(false);
                }
                
            }
        }

        CalcTangents(this);


        //for (int faceid = 0; faceid < triangles.size(); ++faceid)
        //{
        //    vector<Vertex> triangle;
        //    for (int i = 0; i < 3; ++i)
        //    {
        //        Vertex v;
        //        int index = load_buf.triangles[faceid][i];
        //        v.position = load_buf.vertexBuf[index].position;
        //        v.tangent = load_buf.vertexBuf[index].tangent;
        //        v.texcoord = load_buf.texcoords[index];
        //        v.normal = load_buf.normals[index];
        //        triangle.push_back(v);
        //    }
        //    triangles.push_back(triangle);
        //}
    }



    void CalcTangents(Mesh* mesh)
    {
        iface.m_getNumFaces = get_num_faces;
        iface.m_getNumVerticesOfFace = get_num_vertices_of_face;

        iface.m_getNormal = get_normal;
        iface.m_getPosition = get_position;
        iface.m_getTexCoord = get_tex_coords;
        iface.m_setTSpaceBasic = set_tspace_basic;

        context.m_pInterface = &iface;

        context.m_pUserData = mesh;
        genTangSpaceDefault(&this->context);
    }

    SMikkTSpaceInterface iface{};
    SMikkTSpaceContext context{};


    static int get_num_faces(const SMikkTSpaceContext* context)
    {
        Mesh* mesh = static_cast<Mesh*> (context->m_pUserData);
        return mesh->triangles.size();
    }

    static int get_num_vertices_of_face(const SMikkTSpaceContext* context, int iFace)
    {
        return 3;
    }

    static void get_position(const SMikkTSpaceContext* context, float outpos[], int iFace, int iVert)
    {
        Mesh* mesh = static_cast<Mesh*> (context->m_pUserData);

        auto position = mesh->triangles[iFace][iVert].position;

        outpos[0] = position.x;
        outpos[1] = position.y;
        outpos[2] = position.z;
    }

    static void get_normal(const SMikkTSpaceContext* context, float outnormal[], int iFace, int iVert)
    {
        Mesh* mesh = static_cast<Mesh*> (context->m_pUserData);
        auto normal = mesh->triangles[iFace][iVert].normal;

        outnormal[0] = normal.x;
        outnormal[1] = normal.y;
        outnormal[2] = normal.z;
    }

    static void get_tex_coords(const SMikkTSpaceContext* context, float outuv[], int iFace, int iVert)
    {
        Mesh* mesh = static_cast<Mesh*> (context->m_pUserData);
        auto texcoord = mesh->triangles[iFace][iVert].texcoord;


        outuv[0] = texcoord.x;
        outuv[1] = texcoord.y;
    }

    static void set_tspace_basic(const SMikkTSpaceContext* context, const float tangentu[], float fSign, int iFace, int iVert)
    {
        Mesh* mesh = static_cast<Mesh*> (context->m_pUserData);
        mesh->triangles[iFace][iVert].tangent.x = tangentu[0];
        mesh->triangles[iFace][iVert].tangent.y = tangentu[1];
        mesh->triangles[iFace][iVert].tangent.z = tangentu[2];
        mesh->triangles[iFace][iVert].tangent.w = fSign;
    }
};