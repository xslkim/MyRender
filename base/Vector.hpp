#pragma once
#include <math.h>
#include <assert.h>
#include <algorithm>

#include <vector>
#include <ostream>
#include "Utils.hpp"
//---------------------------------------------------------------------
// 数学库：矢量定义
//---------------------------------------------------------------------

// 通用矢量：N 是矢量维度，T 为数据类型
template <size_t N, typename T> 
struct Vector 
{
	T m[N];    // 元素数组
	inline Vector() {
		for (size_t i = 0; i < N; i++) m[i] = T(); 
	}

	inline Vector(const T* ptr) { 
		for (size_t i = 0; i < N; i++) m[i] = ptr[i]; 
	}

	inline Vector(const Vector<N, T>& u) {
		for (size_t i = 0; i < N; i++) m[i] = u.m[i]; 
	}

	inline Vector(const std::initializer_list<T>& u) {
		auto it = u.begin(); for (size_t i = 0; i < N; i++) m[i] = *it++;
	}

	inline const T& operator[] (size_t i) const {
		assert(i < N); return m[i]; 
	}

	inline T& operator[] (size_t i) {
		assert(i < N); return m[i]; 
	}

	inline void load(const T* ptr) {
		for (size_t i = 0; i < N; i++) m[i] = ptr[i]; 
	}

	inline void save(T* ptr) { 
		for (size_t i = 0; i < N; i++) ptr[i] = m[i]; 
	}
};

// 特化二维矢量
template <typename T> struct Vector<2, T> {
	union {
		struct { T x, y; };    // 元素别名
		struct { T u, v; };    // 元素别名
		T m[2];                // 元素数组
	};
	inline Vector() : x(0), y(0) {}
	inline Vector(T X, T Y) : x(X), y(Y) {}
	inline Vector(T v) : x(v), y(v) {}
	inline Vector(int v) : x(v), y(v) {}
	inline Vector(const Vector<2, T>& u) : x(u.x), y(u.y) {}
	inline Vector(const T* ptr) : x(ptr[0]), y(ptr[1]) {}
	inline const T& operator[] (size_t i) const { assert(i < 2); return m[i]; }
	inline T& operator[] (size_t i) { assert(i < 2); return m[i]; }

	inline Vector<2, T> operator+ (T v) const { return Vector<2, T>(x + v, y + v); }
	inline Vector<2, T> operator- (Vector<2, T> v) const { return Vector<2, T>(x - v.x, y - v.y); }
	inline Vector<2, T> operator/ (T v) const { return Vector<2, T>(x / v, y / v); }
	inline Vector<2, T> operator* (Vector<2, T> v) const { return Vector<2, T>(x*v.x, y*v.y); }
	friend Vector<2, T> operator/ (T v, Vector<2, T> self) { return Vector<2, T>(v / self.x, v / self.y); }
	inline void load(const T* ptr) { for (size_t i = 0; i < 2; i++) m[i] = ptr[i]; }
	inline void save(T* ptr) { for (size_t i = 0; i < 2; i++) ptr[i] = m[i]; }

	inline Vector<2, T> get_xy() const { return Vector<2, T>(x, y); }
	inline void set_xy(Vector<2, T> v) { x = v.x; y = v.y; }
	__declspec(property(get = get_xy, put = set_xy)) Vector<2, T> xy;


	inline Vector<3, T> xy1() const { return Vector<3, T>(x, y, 1); }
	inline Vector<4, T> xy11() const { return Vector<4, T>(x, y, 1, 1); }
};


// 特化三维矢量
template <typename T> struct Vector<3, T> {
	union {
		struct { T x, y, z; };    // 元素别名
		struct { T r, g, b; };    // 元素别名
		T m[3];                   // 元素数组
	};
	inline Vector() : x(0), y(0), z(0) {}
	inline Vector(T X, T Y, T Z) : x(X), y(Y), z(Z) {}
	inline Vector(T v) : x(v), y(v), z(v) {}
	inline Vector(int v) : x(v), y(v), z(v) {}
	inline Vector(const Vector<3, T>& u) : x(u.x), y(u.y), z(u.z) {}
	inline Vector(const Vector<2, T>& u, T Z) : x(u.x), y(u.y), z(Z) {}
	inline Vector(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}
	inline const T& operator[] (size_t i) const { assert(i < 3); return m[i]; }
	inline T& operator[] (size_t i) { assert(i < 3); return m[i]; }

	inline Vector<3, T> operator+ (T v) const { return Vector<3, T>(x + v, y + v, z + v); }
	inline Vector<3, T> operator- (T v) const { return Vector<3, T>(x - v, y - v, z - v); }
	inline Vector<3, T> operator-() const { return Vector<3, T>(-x, -y, -z); }
	inline Vector<3, T> operator- (Vector<3, T> v) const { return Vector<3, T>(x - v.x, y - v.y, z - v.z); }
	inline Vector<3, T> operator/ (T v) const { return Vector<3, T>(x / v, y / v, z / v); }
	inline Vector<3, T> operator* (Vector<3, T> v) const { return Vector<3, T>(x * v.x, y * v.y, z * v.z); }
	friend Vector<3, T> operator/ (T v, Vector<3, T> self) { return Vector<3, T>(v / self.x, v / self.y, v /self.z); }

	inline void load(const T* ptr) { for (size_t i = 0; i < 3; i++) m[i] = ptr[i]; }
	inline void save(T* ptr) { for (size_t i = 0; i < 3; i++) ptr[i] = m[i]; }

	inline Vector<2, T> get_xy() const { return Vector<2, T>(x, y); }
	inline void set_xy(Vector<2, T> v) { x = v.x; y = v.y; }
	__declspec(property(get = get_xy, put = set_xy)) Vector<2, T> xy;

	inline Vector<3, T> get_xyz() const { return Vector<3, T>(x, y, z); }
	inline void set_xyz(Vector<3, T> v) { x = v.x; y = v.y; z = v.z; }
	__declspec(property(get = get_xyz, put = set_xyz)) Vector<3, T> xyz;

	inline Vector<3, T> get_rrr() const { return Vector<3, T>(x, x, x); }
	__declspec(property(get = get_rrr)) Vector<3, T> rrr;

	inline Vector<4, T> xyz1() const { return Vector<4, T>(x, y, z, 1); }
};


// 特化四维矢量
template <typename T> struct Vector<4, T> {
	union {
		struct { T x, y, z, w; };    // 元素别名
		struct { T r, g, b, a; };    // 元素别名
		T m[4];                      // 元素数组
	};
	inline Vector() : x(0), y(0), z(0), w(0) {}
	inline Vector(T X, T Y, T Z, T W) : x(X), y(Y), z(Z), w(W) {}
	inline Vector(T v) : x(v), y(v), z(v), w(v) {}
	inline Vector(int v) : x(v), y(v), z(v), w(v) {}
	inline Vector(const Vector<4, T>& u) : x(u.x), y(u.y), z(u.z), w(u.w) {}
	inline Vector(const Vector<3, T>& u, T w) : x(u.x), y(u.y), z(u.z), w(w) {}
	inline Vector(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) {}
	inline const T& operator[] (size_t i) const { assert(i < 4); return m[i]; }
	inline T& operator[] (size_t i) { assert(i < 4); return m[i]; }
	inline void load(const T* ptr) { for (size_t i = 0; i < 4; i++) m[i] = ptr[i]; }
	inline void save(T* ptr) { for (size_t i = 0; i < 4; i++) ptr[i] = m[i]; }

	inline Vector<2, T> get_xy() const { return Vector<2, T>(x, y); }
	inline void set_xy(Vector<2, T> v) { x = v.x; y = v.y; }
	__declspec(property(get = get_xy, put = set_xy)) Vector<2, T> xy;

	inline Vector<2, T> get_zw() const { return Vector<2, T>(z, w); }
	inline void set_zw(Vector<2, T> v) { z = v.x; w = v.y; }
	__declspec(property(get = get_zw, put = set_zw)) Vector<2, T> zw;

	inline Vector<3, T> get_xyz() const { return Vector<3, T>(x, y, z); }
	inline void set_xyz(Vector<3, T> v) { x = v.x; y = v.y; z = v.z; }
	__declspec(property(get = get_xyz, put = set_xyz)) Vector<3, T> xyz;
	__declspec(property(get = get_xyz, put = set_xyz)) Vector<3, T> rgb;

	inline Vector<3, T> get_aaa() const { return Vector<3, T>(w, w, w); }
	__declspec(property(get = get_aaa)) Vector<3, T> aaa;


	inline Vector<4, T> get_xyzw() const { return *this; }
	__declspec(property(get = get_xyzw)) Vector<4, T> xyzw;
};

// 类型别名
typedef Vector<2, float>  Vec2f;
typedef Vector<2, double> Vec2d;
typedef Vector<2, int>    Vec2i;
typedef Vector<3, float>  Vec3f;
typedef Vector<3, double> Vec3d;
typedef Vector<3, int>    Vec3i;
typedef Vector<4, float>  Vec4f;
typedef Vector<4, double> Vec4d;
typedef Vector<4, int>    Vec4i;


typedef Vector<2, float> float2;
typedef Vector<3, float> float3;
typedef Vector<4, float> float4;

typedef float half;
typedef float real;
typedef float3 real3;
typedef float4 real4;
typedef float3 half3;
typedef float4 half4;
typedef Vector<4, float> Color;




// = (a == b) ? true : false
template <size_t N, typename T>
inline bool operator == (const Vector<N, T>& a, const Vector<N, T>& b) {
	for (size_t i = 0; i < N; i++) if (a[i] != b[i]) return false;
	return true;
}

// = (a != b)? true : false
template <size_t N, typename T>
inline bool operator != (const Vector<N, T>& a, const Vector<N, T>& b) {
	return !(a == b);
}

// = a + b
template <size_t N, typename T>
inline Vector<N, T> operator + (const Vector<N, T>& a, const Vector<N, T>& b) {
	Vector<N, T> c;
	for (size_t i = 0; i < N; i++) c[i] = a[i] + b[i];
	return c;
}

// = a - b
template <size_t N, typename T>
inline Vector<N, T> operator - (const Vector<N, T>& a, const Vector<N, T>& b) {
	Vector<N, T> c;
	for (size_t i = 0; i < N; i++) c[i] = a[i] - b[i];
	return c;
}

// = a * b，不是点乘也不是叉乘，而是各个元素分别相乘，色彩计算时有用
template <size_t N, typename T>
inline Vector<N, T> operator * (const Vector<N, T>& a, const Vector<N, T>& b) {
	Vector<N, T> c;
	for (size_t i = 0; i < N; i++) c[i] = a[i] * b[i];
	return c;
}

// = a / b，各个元素相除
template <size_t N, typename T>
inline Vector<N, T> operator / (const Vector<N, T>& a, const Vector<N, T>& b) {
	Vector<N, T> c;
	for (size_t i = 0; i < N; i++) c[i] = a[i] / b[i];
	return c;
}

// = a * x
template <size_t N, typename T>
inline Vector<N, T> operator * (const Vector<N, T>& a, T x) {
	Vector<N, T> b;
	for (size_t i = 0; i < N; i++) b[i] = a[i] * x;
	return b;
}

// = x * a
template <size_t N, typename T>
inline Vector<N, T> operator * (T x, const Vector<N, T>& a) {
	Vector<N, T> b;
	for (size_t i = 0; i < N; i++) b[i] = a[i] * x;
	return b;
}

// = a / x
template <size_t N, typename T>
inline Vector<N, T> operator / (const Vector<N, T>& a, T x) {
	Vector<N, T> b;
	for (size_t i = 0; i < N; i++) b[i] = a[i] / x;
	return b;
}

// = x / a
template <size_t N, typename T>
inline Vector<N, T> operator / (T x, const Vector<N, T>& a) {
	Vector<N, T> b;
	for (size_t i = 0; i < N; i++) b[i] = x / a[i];
	return b;
}

// a += b
template <size_t N, typename T>
inline Vector<N, T>& operator += (Vector<N, T>& a, const Vector<N, T>& b) {
	for (size_t i = 0; i < N; i++) a[i] += b[i];
	return a;
}

// a -= b
template <size_t N, typename T>
inline Vector<N, T>& operator -= (Vector<N, T>& a, const Vector<N, T>& b) {
	for (size_t i = 0; i < N; i++) a[i] -= b[i];
	return a;
}

// a *= b
template <size_t N, typename T>
inline Vector<N, T>& operator *= (Vector<N, T>& a, const Vector<N, T>& b) {
	for (size_t i = 0; i < N; i++) a[i] *= b[i];
	return a;
}

// a /= b
template <size_t N, typename T>
inline Vector<N, T>& operator /= (Vector<N, T>& a, const Vector<N, T>& b) {
	for (size_t i = 0; i < N; i++) a[i] /= b[i];
	return a;
}

// a *= x
template <size_t N, typename T>
inline Vector<N, T>& operator *= (Vector<N, T>& a, T x) {
	for (size_t i = 0; i < N; i++) a[i] *= x;
	return a;
}

// a /= x
template <size_t N, typename T>
inline Vector<N, T>& operator /= (Vector<N, T>& a, T x) {
	for (size_t i = 0; i < N; i++) a[i] /= x;
	return a;
}



// = |a| ^ 2
template<size_t N, typename T>
inline T vector_length_square(const Vector<N, T>& a) {
	T sum = 0;
	for (size_t i = 0; i < N; i++) sum += a[i] * a[i];
	return sum;
}

// = |a|
template<size_t N, typename T>
inline T vector_length(const Vector<N, T>& a) {
	return sqrt(vector_length_square(a));
}

// = |a| , 特化 float 类型，使用 sqrtf
template<size_t N>
inline float vector_length(const Vector<N, float>& a) {
	return sqrtf(vector_length_square(a));
}

// = a / |a|
template<size_t N, typename T>
inline Vector<N, T> vector_normalize(const Vector<N, T>& a) {
	return a / vector_length(a);
}

// 矢量点乘
template<size_t N, typename T>
inline T vector_dot(const Vector<N, T>& a, const Vector<N, T>& b) {
	T sum = 0;
	for (size_t i = 0; i < N; i++) sum += a[i] * b[i];
	return sum;
}

// 二维矢量叉乘，得到标量
template<typename T>
inline T vector_cross(const Vector<2, T>& a, const Vector<2, T>& b) {
	return a.x * b.y - a.y * b.x;
}

// 三维矢量叉乘，得到新矢量
template<typename T>
inline Vector<3, T> vector_cross(const Vector<3, T>& a, const Vector<3, T>& b) {
	return Vector<3, T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// 四维矢量叉乘：前三维叉乘，后一位保留
template<typename T>
inline Vector<4, T> vector_cross(const Vector<4, T>& a, const Vector<4, T>& b) {
	return Vector<4, T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, a.w);
}

// = a + (b - a) * t
template<size_t N, typename T>
inline Vector<N, T> vector_lerp(const Vector<N, T>& a, const Vector<N, T>& b, float t) {
	return a + (b - a) * t;
}

// 各个元素取最大值
template<size_t N, typename T>
inline Vector<N, T> vector_max(const Vector<N, T>& a, const Vector<N, T>& b) {
	Vector<N, T> c;
	for (size_t i = 0; i < N; i++) c[i] = (a[i] > b[i]) ? a[i] : b[i];
	return c;
}

// 各个元素取最小值
template<size_t N, typename T>
inline Vector<N, T> vector_min(const Vector<N, T>& a, const Vector<N, T>& b) {
	Vector<N, T> c;
	for (size_t i = 0; i < N; i++) c[i] = (a[i] < b[i]) ? a[i] : b[i];
	return c;
}

// 将矢量的值控制在 minx/maxx 范围内
template<size_t N, typename T>
inline Vector<N, T> vector_between(const Vector<N, T>& minx, const Vector<N, T>& maxx, const Vector<N, T>& x) {
	return vector_min(vector_max(minx, x), maxx);
}

// 判断矢量是否接近
template<size_t N, typename T>
inline bool vector_near(const Vector<N, T>& a, const Vector<N, T>& b, T dist) {
	return (vector_length_square(a - b) <= dist);
}

// 判断两个单精度矢量是否近似
template<size_t N>
inline bool vector_near_equal(const Vector<N, float>& a, const Vector<N, float>& b, float e = 0.0001) {
	return vector_near(a, b, e);
}

// 判断两个双精度矢量是否近似
template<size_t N>
inline bool vector_near_equal(const Vector<N, double>& a, const Vector<N, double>& b, double e = 0.0000001) {
	return vector_near(a, b, e);
}

// 矢量值元素范围裁剪
template<size_t N, typename T>
inline Vector<N, T> vector_clamp(const Vector<N, T>& a, T minx = 0, T maxx = 1) {
	Vector<N, T> b;
	for (size_t i = 0; i < N; i++) {
		T x = (a[i] < minx) ? minx : a[i];
		b[i] = (x > maxx) ? maxx : x;
	}
	return b;
}

Vec2f vec2_lerp(Vec2f a, Vec2f b, float t) {
	float x = lerp(a.x, b.x, t);
	float y = lerp(a.y, b.y, t);
	return Vec2f(x, y);
}


float dot(float3 a, float3 b)
{
	return vector_dot(a, b);
}

Vec3f vec3_min(Vec3f a, Vec3f b) {
	float x = std::min(a.x, b.x);
	float y = std::min(a.y, b.y);
	float z = std::min(a.z, b.z);
	return Vec3f(x, y, z);
}

Vec3f vec3_max(Vec3f a, Vec3f b) {
	float x = std::max(a.x, b.x);
	float y = std::max(a.y, b.y);
	float z = std::max(a.z, b.z);
	return Vec3f(x, y, z);
}

Vec3f vec3_add(Vec3f a, Vec3f b) {
	return Vec3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3f vec3_sub(Vec3f a, Vec3f b) {
	return Vec3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3f vec3_mul(Vec3f v, float factor) {
	return Vec3f(v.x * factor, v.y * factor, v.z * factor);
}

Vec3f vec3_div(Vec3f v, float divisor) {
	return vec3_mul(v, 1 / divisor);
}

Vec3f vec3_new(float x, float y, float z) {
	Vec3f v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

float vec3_dot(Vec3f a, Vec3f b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vec3_length(Vec3f v) {
	return (float)sqrt(vec3_dot(v, v));
}

Vec3f vec3_from_vec4(Vec4f v) {
	return vec3_new(v.x, v.y, v.z);
}

Vec3f vec3_normalize(Vec3f v) {
	return vec3_div(v, vec3_length(v));
}

Vec3f vec3_lerp(Vec3f a, Vec3f b, float t) {
	float x = lerp(a.x, b.x, t);
	float y = lerp(a.y, b.y, t);
	float z = lerp(a.z, b.z, t);
	return vec3_new(x, y, z);
}

Vec4f vec4_new(float x, float y, float z, float w) {
	Vec4f v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

Vec4f vec4_from_vec3(Vec3f v, float w) {
	return vec4_new(v.x, v.y, v.z, w);
}

Vec4f vec4_add(Vec4f a, Vec4f b) {
	return vec4_new(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

Vec4f vec4_sub(Vec4f a, Vec4f b) {
	return vec4_new(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

Vec4f vec4_mul(Vec4f v, float factor) {
	return vec4_new(v.x * factor, v.y * factor, v.z * factor, v.w * factor);
}

Vec4f vec4_div(Vec4f v, float divisor) {
	return vec4_mul(v, 1 / divisor);
}

Vec4f vec4_lerp(Vec4f a, Vec4f b, float t) {
	float x = lerp(a.x, b.x, t);
	float y = lerp(a.y, b.y, t);
	float z = lerp(a.z, b.z, t);
	float w = lerp(a.w, b.w, t);
	return vec4_new(x, y, z, w);
}

Vec4f vec4_saturate(Vec4f v) {
	float x = float_saturate(v.x);
	float y = float_saturate(v.y);
	float z = float_saturate(v.z);
	float w = float_saturate(v.w);
	return vec4_new(x, y, z, w);
}

Vec4f vec4_modulate(Vec4f a, Vec4f b) {
	return vec4_new(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}


// 矢量转整数颜色
inline static uint32_t vector_to_color(const Vec4f& color) {
	uint32_t r = (uint32_t)Between<int>(0, 255, (int)(color.r * 255.0f));
	uint32_t g = (uint32_t)Between<int>(0, 255, (int)(color.g * 255.0f));
	uint32_t b = (uint32_t)Between<int>(0, 255, (int)(color.b * 255.0f));
	uint32_t a = (uint32_t)Between<int>(0, 255, (int)(color.a * 255.0f));
	return (r << 16) | (g << 8) | b | (a << 24);
}

// 矢量转换整数颜色
inline static uint32_t vector_to_color(const Vec3f& color) {
	return vector_to_color(color.xyz1());
}

// 整数颜色到矢量
inline static Vec4f vector_from_color(uint32_t rgba) {
	Vec4f out;
	out.r = ((rgba >> 16) & 0xff) / 255.0f;
	out.g = ((rgba >> 8) & 0xff) / 255.0f;
	out.b = ((rgba >> 0) & 0xff) / 255.0f;
	out.a = ((rgba >> 24) & 0xff) / 255.0f;
	return out;
}

float3 lerp(float3 a, float3 b, float t)
{
	return vector_lerp<3, float>(a, b, t);
}

real Max3(real a, real b, real c)
{
	return max(max(a, b), c);
}



