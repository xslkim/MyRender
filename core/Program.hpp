#pragma once
#include "Attributes.hpp"
#include "Varyings.hpp"

typedef void VertexShader(const Attributes* attributes, Varyings* varyings);
typedef half4 FragmentShader(Varyings& varyings);

//ȷ��ÿһ֡��Ⱦ�������㹻��attributes��varyings, һ���򵥵��ڴ��
const int kBufferSize = 1024 * 1024;

Attributes* g_attributes_buffer = nullptr;
int g_attributes_buffer_index = 0;

Attributes* GetOneAttributes()
{
    //���̰߳�ȫ�Ļ�ȡ��� attributes
    if (g_attributes_buffer == nullptr)
    {
        g_attributes_buffer = new Attributes[kBufferSize];
    }
    return &g_attributes_buffer[g_attributes_buffer_index++];
}


Varyings* g_varyings_buffer = nullptr;
int g_varyings_buffer_index = 0;
Varyings* GetOneVaryings()
{
    if (g_varyings_buffer == nullptr)
    {
        g_varyings_buffer = new Varyings[kBufferSize];
    }
    return &g_varyings_buffer[g_varyings_buffer_index++];
}

void ResetProgramBuffer()
{
    g_attributes_buffer_index = 0;
    g_varyings_buffer_index = 0;
}


