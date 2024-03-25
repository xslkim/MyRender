#pragma once
#include <string>

using namespace std;

#define EPSILON 0.001
#define PI 3.1415927f
#define TO_RADIANS(degrees) ((PI / 180) * (degrees))
#define TO_DEGREES(radians) ((180 / PI) * (radians))

typedef unsigned int uint;

//---------------------------------------------------------------------
// 工具函数
//---------------------------------------------------------------------
template<typename T> inline T Abs(T x) { return (x < 0) ? (-x) : x; }
template<typename T> inline T Max(T x, T y) { return (x < y) ? y : x; }
template<typename T> inline T Min(T x, T y) { return (x > y) ? y : x; }

template<typename T> inline bool NearEqual(T x, T y, T error) {
	return (Abs(x - y) < error);
}

template<typename T> 
inline T Between(T xmin, T xmax, T x) {
	return Min(Max(xmin, x), xmax);
}

// 截取 [0, 1] 的范围
template<typename T> inline T Saturate(T x) {
	return Between<T>(0, 1, x);
}


float float_clamp(float f, float min, float max) {
	return f < min ? min : (f > max ? max : f);
}

float float_saturate(float f) {
	return f < 0 ? 0 : (f > 1 ? 1 : f);
}

float uchar_to_float(unsigned char value) {
	return value / 255.0f;
}

unsigned char float_to_uchar(float value) {
	return (unsigned char)(value * 255);
}

float float_srgb2linear(float value) {
	return (float)pow(value, 2.2);
}

float float_linear2srgb(float value) {
	return (float)pow(value, 1 / 2.2);
}

/*
 * for aces filmic tone mapping curve, see
 * https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
 */
float float_aces(float value) {
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	value = (value * (a * value + b)) / (value * (c * value + d) + e);
	return float_saturate(value);
}


static const int LINE_SIZE = 256;
int equals_to(const char* str1, const char* str2) {
	return strcmp(str1, str2) == 0;
}


const char* wrap_path(const char* path) {
	if (equals_to(path, "null")) {
		return NULL;
	}
	else {
		return path;
	}
}

int wrap_knob(const char* knob) {
	if (equals_to(knob, "on")) {
		return 1;
	}
	else {
		assert(equals_to(knob, "off"));
		return 0;
	}
}

