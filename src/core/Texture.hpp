#pragma once
#include <string>
#include <map>
#include "Vector.hpp"
#include "Image.hpp"
#include "Config.hpp"

struct SamplerState
{
    std::string filter; // Point / Linear / Trilinear
    std::string wrap;   // Clamp / Repeat / Mirror / MirrorOnce
};

class Texture2D
{
public:
    Texture2D() : width(0), height(0), buffer(nullptr) {}
    ~Texture2D() { free(buffer); }

    void Load(const std::string& filename, bool is_normal)
    {
        Image image(filename);
        ldr_image_to_texture(&image);
        if (!is_normal)
            srgb_to_linear();
    }

    float4 SamplerPoint(float x, float y) const
    {
        float u = x - (float)floor(x);
        float v = y - (float)floor(y);
        int   c = (int)((width  - 1) * u);
        int   r = (int)((height - 1) * v);
        return buffer[r * width + c];
    }

    // Bilinear filtering with Repeat wrap. Samples the four texels around the
    // continuous coordinate and blends by the fractional position.
    float4 SamplerLinear(float x, float y) const
    {
        float u = x - (float)floor(x);
        float v = y - (float)floor(y);

        // Texel-center sample space: subtract 0.5 so integer coords land on centers.
        float fx = u * width  - 0.5f;
        float fy = v * height - 0.5f;
        int   x0 = (int)floor(fx);
        int   y0 = (int)floor(fy);
        float tx = fx - x0;
        float ty = fy - y0;

        auto wrap = [](int i, int n) { i %= n; return i < 0 ? i + n : i; };
        int x0w = wrap(x0,     width),  x1w = wrap(x0 + 1, width);
        int y0w = wrap(y0,     height), y1w = wrap(y0 + 1, height);

        const Vec4f& c00 = buffer[y0w * width + x0w];
        const Vec4f& c10 = buffer[y0w * width + x1w];
        const Vec4f& c01 = buffer[y1w * width + x0w];
        const Vec4f& c11 = buffer[y1w * width + x1w];

        Vec4f top = c00 * (1 - tx) + c10 * tx;
        Vec4f bot = c01 * (1 - tx) + c11 * tx;
        return top * (1 - ty) + bot * ty;
    }

private:
    int     width, height;
    Vec4f*  buffer;

    void texture_create(int w, int h)
    {
        assert(w > 0 && h > 0);
        width  = w;
        height = h;
        buffer = (Vec4f*)malloc(sizeof(Vec4f) * w * h);
        memset(buffer, 0, sizeof(Vec4f) * w * h);
    }

    void ldr_image_to_texture(Image* image)
    {
        texture_create(image->width, image->height);
        int num_pixels = image->width * image->height;
        for (int i = 0; i < num_pixels; i++) {
            unsigned char* pixel = &image->ldr_buffer[i * image->channels];
            Vec4f texel = { 0, 0, 0, 1 };
            if      (image->channels == 1) { texel.x = texel.y = texel.z = uchar_to_float(pixel[0]); }
            else if (image->channels == 2) { texel.x = texel.y = texel.z = uchar_to_float(pixel[0]); texel.w = uchar_to_float(pixel[1]); }
            else if (image->channels == 3) { texel.x = uchar_to_float(pixel[0]); texel.y = uchar_to_float(pixel[1]); texel.z = uchar_to_float(pixel[2]); }
            else                           { texel.x = uchar_to_float(pixel[0]); texel.y = uchar_to_float(pixel[1]); texel.z = uchar_to_float(pixel[2]); texel.w = uchar_to_float(pixel[3]); }
            buffer[i] = texel;
        }
    }

    void srgb_to_linear()
    {
        int num_pixels = width * height;
        for (int i = 0; i < num_pixels; i++) {
            buffer[i].x = float_srgb2linear(buffer[i].x);
            buffer[i].y = float_srgb2linear(buffer[i].y);
            buffer[i].z = float_srgb2linear(buffer[i].z);
        }
    }
};


class TextureCache
{
public:
    static TextureCache& Get()
    {
        static TextureCache instance;
        return instance;
    }

    Texture2D* GetTexture(const std::string& file_name, bool is_normal = false)
    {
        if (file_name.empty())
            return nullptr;

        auto it = _textures.find(file_name);
        if (it != _textures.end())
            return it->second;

        Texture2D* tex = new Texture2D();
        tex->Load(Config::scene_path + file_name, is_normal);
        _textures[file_name] = tex;
        return tex;
    }

    ~TextureCache() { /* TODO: cleanup */ }

private:
    std::map<std::string, Texture2D*> _textures;
};
