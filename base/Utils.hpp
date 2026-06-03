#pragma once
#include <math.h>
#include <assert.h>

#define EPSILON 0.001
#define PI      3.1415927f

typedef unsigned int uint;

static const int LINE_SIZE = 256;

template<typename T> inline T Abs(T x)                            { return (x < 0) ? (-x) : x; }
template<typename T> inline T Max(T x, T y)                       { return (x < y) ? y : x; }
template<typename T> inline T Min(T x, T y)                       { return (x > y) ? y : x; }
template<typename T> inline bool NearEqual(T x, T y, T error)     { return (Abs(x - y) < error); }
template<typename T> inline T Between(T xmin, T xmax, T x)        { return Min(Max(xmin, x), xmax); }
template<typename T> inline T Saturate(T x)                       { return Between<T>(0, 1, x); }

inline float float_clamp(float f, float lo, float hi)  { return f < lo ? lo : (f > hi ? hi : f); }
inline float float_saturate(float f)                   { return f < 0 ? 0 : (f > 1 ? 1 : f); }
inline float uchar_to_float(unsigned char v)           { return v / 255.0f; }
inline unsigned char float_to_uchar(float v)           { return (unsigned char)(v * 255); }
inline float float_srgb2linear(float v)                { return (float)pow(v, 2.2); }
inline float float_linear2srgb(float v)                { return (float)pow(v, 1.0 / 2.2); }

// ACES filmic tone mapping: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
inline float float_aces(float v)
{
    const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    v = (v * (a * v + b)) / (v * (c * v + d) + e);
    return float_saturate(v);
}
