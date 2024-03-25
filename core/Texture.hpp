#pragma once
#include "Vector.hpp"
#include "Image.hpp"
#include "Config.hpp"
#include <map>

struct SamplerState
{
    string filter;//*“Point”、“Linear”或“Trilinear”（必需）设置纹理过滤模式。
    string wrap;// *“Clamp”、“Repeat”、“Mirror”或“MirrorOnce”（必需）设置纹理包裹模式。
       //* 可根据每个轴(UVW) 来指定包裹模式，例如"ClampU_RepeatV"。
       // * “Compare”（可选）设置用于深度比较的采样器；与 HLSL SamplerComparisonState 类型和 SampleCmp / SampleCmpLevelZero 函数配合使用。
};

class Texture2D
{
public:
	Texture2D() :width(0),height(0),buffer(NULL){}

    ~Texture2D() { free(buffer); }

    //纹理统一为线性值
    void Load(const string& filename, bool is_normal) {
        Image image(filename);
        ldr_image_to_texture(&image);
        if (!is_normal)
        {
            srgb_to_linear();
        }
    }

    float4 SamplerPoint(float x, float y) const {
        float u = x - (float)floor(x);
        float v = y - (float)floor(y);
        int c = (int)((width - 1) * u);
        int r = (int)((height - 1) * v);
        int index = r * width + c;
        return buffer[index];
    }

    float4 SamplerLinear(float x, float y) const{
        float u = x - (float)floor(x);
        float v = y - (float)floor(y);
        int c = (int)((width - 1) * u);
        int r = (int)((height - 1) * v);
        int index = r * width + c;
        return buffer[index];
    }

private:
    int width, height;
    Vec4f* buffer;

    void texture_create(int width, int height)
    {
        int buffer_size = sizeof(Vec4f) * width * height;
        assert(width > 0 && height > 0);
        this->width = width;
        this->height = height;
        this->buffer = (Vec4f*)malloc(buffer_size);
        memset(this->buffer, 0, buffer_size);
    }

    void ldr_image_to_texture(Image* image) {
        texture_create(image->width, image->height);

        int num_pixels = image->width * image->height;
        int i;

        for (i = 0; i < num_pixels; i++) {
            unsigned char* pixel = &image->ldr_buffer[i * image->channels];
            Vec4f texel = { 0, 0, 0, 1 };
            if (image->channels == 1) {             /* GL_LUMINANCE */
                texel.x = texel.y = texel.z = uchar_to_float(pixel[0]);
            }
            else if (image->channels == 2) {      /* GL_LUMINANCE_ALPHA */
                texel.x = texel.y = texel.z = uchar_to_float(pixel[0]);
                texel.w = uchar_to_float(pixel[1]);
            }
            else if (image->channels == 3) {      /* GL_RGB */
                texel.x = uchar_to_float(pixel[0]);
                texel.y = uchar_to_float(pixel[1]);
                texel.z = uchar_to_float(pixel[2]);
            }
            else {                                /* GL_RGBA */
                texel.x = uchar_to_float(pixel[0]);
                texel.y = uchar_to_float(pixel[1]);
                texel.z = uchar_to_float(pixel[2]);
                texel.w = uchar_to_float(pixel[3]);
            }
            this->buffer[i] = texel;
        }
    }

    void srgb_to_linear() {
        int num_pixels = this->width * this->height;
        int i;

        for (i = 0; i < num_pixels; i++) {
            Vec4f* pixel = &this->buffer[i];
            pixel->x = float_srgb2linear(pixel->x);
            pixel->y = float_srgb2linear(pixel->y);
            pixel->z = float_srgb2linear(pixel->z);
        }
    }

    void linear_to_srgb() {
        int num_pixels = this->width * height;
        int i;

        for (i = 0; i < num_pixels; i++) {
            Vec4f* pixel = &this->buffer[i];
            pixel->x = float_linear2srgb(float_aces(pixel->x));
            pixel->y = float_linear2srgb(float_aces(pixel->y));
            pixel->z = float_linear2srgb(float_aces(pixel->z));
        }
    }
};

class TextureCatch
{
public:
    static TextureCatch& Get() {
        static TextureCatch _instance;
        return _instance;
    }

    Texture2D* GetTexture(const string& file_name, bool is_normal = false)
    {
        if (file_name.empty())
        {
            return nullptr;
        }

        Texture2D* tex = GetTex(file_name);
        if(tex == nullptr)
        {
            Load(file_name, is_normal);
            return GetTex(file_name);
        }
        else 
        {
            return tex;
        }
    }

    void Load(const string& name, bool is_normal)
    {
        if (_textures.find(name) == _textures.end())
        {
            Texture2D* tex = new Texture2D();
            tex->Load(Config::scene_path + name, is_normal);
            _textures[name] = tex;
        }
    }
private:
    Texture2D* GetTex(const string& texture)
    {
        auto it = _textures.find(texture);
        if (it != _textures.end())
        {
            return it->second;
        }
        return nullptr;
    }
    map<string, Texture2D*> _textures;
};