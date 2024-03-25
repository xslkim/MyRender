#pragma once
#include "Vector.hpp"
#include <math.h>
//---------------------------------------------------------------------
// 数学库：矩阵
//---------------------------------------------------------------------
template<size_t ROW, size_t COL, typename T> struct Matrix {
	T m[ROW][COL];

	inline Matrix() {
		SetIdentity();
	}

	inline void SetIdentity()
	{
		for (size_t r = 0; r < ROW; r++) {
			for (size_t c = 0; c < COL; c++)
				m[r][c] = (r == c) ? 1 : 0;
		}
	}

	inline Matrix(const Matrix<ROW, COL, T>& src) {
		for (size_t r = 0; r < ROW; r++) {
			for (size_t c = 0; c < COL; c++)
				m[r][c] = src.m[r][c];
		}
	}

	inline Matrix(const std::initializer_list<Vector<COL, T>>& u) {
		auto it = u.begin();
		for (size_t i = 0; i < ROW; i++) SetRow(i, *it++);
	}

	inline Matrix(Vector<3, T> r0, Vector<3, T> r1, Vector<3, T> r2) {
		SetRow(0, r0);
		SetRow(1, r1);
		SetRow(2, r2);
	}

	inline const T* operator [] (size_t row) const { assert(row < ROW); return m[row]; }
	inline T* operator [] (size_t row) { assert(row < ROW); return m[row]; }

	// 取一行
	inline Vector<COL, T> Row(size_t row) const {
		assert(row < ROW);
		Vector<COL, T> a;
		for (size_t i = 0; i < COL; i++) a[i] = m[row][i];
		return a;
	}

	// 取一列
	inline Vector<ROW, T> Col(size_t col) const {
		assert(col < COL);
		Vector<ROW, T> a;
		for (size_t i = 0; i < ROW; i++) a[i] = m[i][col];
		return a;
	}

	// 设置一行
	inline void SetRow(size_t row, const Vector<COL, T>& a) {
		assert(row < ROW);
		for (size_t i = 0; i < COL; i++) m[row][i] = a[i];
	}

	// 设置一列
	inline void SetCol(size_t col, const Vector<ROW, T>& a) {
		assert(col < COL);
		for (size_t i = 0; i < ROW; i++) m[i][col] = a[i];
	}

	// 取得删除某行和某列的子矩阵：子式
	inline Matrix<ROW - 1, COL - 1, T> GetMinor(size_t row, size_t col) const {
		Matrix<ROW - 1, COL - 1, T> ret;
		for (size_t r = 0; r < ROW - 1; r++) {
			for (size_t c = 0; c < COL - 1; c++) {
				ret.m[r][c] = m[r < row ? r : r + 1][c < col ? c : c + 1];
			}
		}
		return ret;
	}

	// 取得转置矩阵
	inline Matrix<COL, ROW, T> Transpose() const {
		Matrix<COL, ROW, T> ret;
		for (size_t r = 0; r < ROW; r++) {
			for (size_t c = 0; c < COL; c++)
				ret.m[c][r] = m[r][c];
		}
		return ret;
	}

	// 取得 0 矩阵
	inline static Matrix<ROW, COL, T> GetZero() {
		Matrix<ROW, COL, T> ret;
		for (size_t r = 0; r < ROW; r++) {
			for (size_t c = 0; c < COL; c++)
				ret.m[r][c] = 0;
		}
		return ret;
	}

	// 取得单位矩阵
	inline static Matrix<ROW, COL, T> GetIdentity() {
		Matrix<ROW, COL, T> ret;
		ret.SetIdentity();
		return ret;
	}
};

typedef Matrix<4, 4, float> Mat4x4f;
typedef Matrix<3, 3, float> Mat3x3f;
typedef Matrix<4, 3, float> Mat4x3f;
typedef Matrix<3, 4, float> Mat3x4f;

typedef Mat4x4f float4x4;
typedef Mat3x3f float3x3;
typedef Mat3x3f half3x3;


//---------------------------------------------------------------------
// 数学库：矩阵运算
//---------------------------------------------------------------------
template<size_t ROW, size_t COL, typename T>
inline bool operator == (const Matrix<ROW, COL, T>& a, const Matrix<ROW, COL, T>& b) {
	for (size_t r = 0; r < ROW; r++) {
		for (size_t c = 0; c < COL; c++) {
			if (a.m[r][c] != b.m[r][c]) return false;
		}
	}
	return true;
}

template<size_t ROW, size_t COL, typename T>
inline bool operator != (const Matrix<ROW, COL, T>& a, const Matrix<ROW, COL, T>& b) {
	return !(a == b);
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator + (const Matrix<ROW, COL, T>& src) {
	return src;
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator - (const Matrix<ROW, COL, T>& src) {
	Matrix<ROW, COL, T> out;
	for (size_t j = 0; j < ROW; j++) {
		for (size_t i = 0; i < COL; i++)
			out.m[j][i] = -src.m[j][i];
	}
	return out;
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator + (const Matrix<ROW, COL, T>& a, const Matrix<ROW, COL, T>& b) {
	Matrix<ROW, COL, T> out;
	for (size_t j = 0; j < ROW; j++) {
		for (size_t i = 0; i < COL; i++)
			out.m[j][i] = a.m[j][i] + b.m[j][i];
	}
	return out;
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator - (const Matrix<ROW, COL, T>& a, const Matrix<ROW, COL, T>& b) {
	Matrix<ROW, COL, T> out;
	for (size_t j = 0; j < ROW; j++) {
		for (size_t i = 0; i < COL; i++)
			out.m[j][i] = a.m[j][i] - b.m[j][i];
	}
	return out;
}

template<size_t ROW, size_t COL, size_t NEWCOL, typename T>
inline Matrix<ROW, NEWCOL, T> operator * (const Matrix<ROW, COL, T>& a, const Matrix<COL, NEWCOL, T>& b) {
	Matrix<ROW, NEWCOL, T> out;
	for (size_t j = 0; j < ROW; j++) {
		for (size_t i = 0; i < NEWCOL; i++) {
			out.m[j][i] = vector_dot(a.Row(j), b.Col(i));
		}
	}
	return out;
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator * (const Matrix<ROW, COL, T>& a, T x) {
	Matrix<ROW, COL, T> out;
	for (size_t j = 0; j < ROW; j++) {
		for (size_t i = 0; i < COL; i++) {
			out.m[j][i] = a.m[j][i] * x;
		}
	}
	return out;
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator / (const Matrix<ROW, COL, T>& a, T x) {
	Matrix<ROW, COL, T> out;
	for (size_t j = 0; j < ROW; j++) {
		for (size_t i = 0; i < COL; i++) {
			out.m[j][i] = a.m[j][i] / x;
		}
	}
	return out;
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator * (T x, const Matrix<ROW, COL, T>& a) {
	return (a * x);
}

template<size_t ROW, size_t COL, typename T>
inline Matrix<ROW, COL, T> operator / (T x, const Matrix<ROW, COL, T>& a) {
	Matrix<ROW, COL, T> out;
	for (size_t j = 0; j < ROW; j++) {
		for (size_t i = 0; i < COL; i++) {
			out.m[j][i] = x / a.m[j][i];
		}
	}
	return out;
}

template<size_t ROW, size_t COL, typename T>
inline Vector<COL, T> operator * (const Vector<ROW, T>& a, const Matrix<ROW, COL, T>& m) {
	Vector<COL, T> b;
	for (size_t i = 0; i < COL; i++)
		b[i] = vector_dot(a, m.Col(i));
	return b;
}

template<size_t ROW, size_t COL, typename T>
inline Vector<ROW, T> operator * (const Matrix<ROW, COL, T>& m, const Vector<COL, T>& a) {
	Vector<ROW, T> b;
	for (size_t i = 0; i < ROW; i++)
		b[i] = vector_dot(a, m.Row(i));
	return b;
}


template<typename T>
inline static float4x4 createMatrix4x4(
	T v00, T v01, T v02, T v03,
	T v10, T v11, T v12, T v13,
	T v20, T v21, T v22, T v23,
	T v30, T v31, T v32, T v33)
{
	Matrix<4, 4, T> mat;
	mat.m[0][0] = v00; mat.m[0][1] = v01; mat.m[0][2] = v02; mat.m[0][3] = v03;
	mat.m[1][0] = v10; mat.m[1][1] = v11; mat.m[1][2] = v12; mat.m[1][3] = v13;
	mat.m[2][0] = v20; mat.m[2][1] = v21; mat.m[2][2] = v22; mat.m[2][3] = v23;
	mat.m[3][0] = v30; mat.m[3][1] = v31; mat.m[3][2] = v32; mat.m[3][3] = v33;
	return mat;
}

template<typename T>
inline static float3x3 createMatrix3x3(
	T v00, T v01, T v02,
	T v10, T v11, T v12,
	T v20, T v21, T v22)
{
	Matrix<3, 3, T> mat;
	mat.m[0][0] = v00; mat.m[0][1] = v01; mat.m[0][2] = v02; 
	mat.m[1][0] = v10; mat.m[1][1] = v11; mat.m[1][2] = v12; 
	mat.m[2][0] = v20; mat.m[2][1] = v21; mat.m[2][2] = v22; 
	return mat;
}

// matrix set to zero
inline static Mat4x4f matrix_set_zero() {
	Mat4x4f m;
	m.m[0][0] = m.m[0][1] = m.m[0][2] = m.m[0][3] = 0.0f;
	m.m[1][0] = m.m[1][1] = m.m[1][2] = m.m[1][3] = 0.0f;
	m.m[2][0] = m.m[2][1] = m.m[2][2] = m.m[2][3] = 0.0f;
	m.m[3][0] = m.m[3][1] = m.m[3][2] = m.m[3][3] = 0.0f;
	return m;
}

// set to identity
inline static Mat4x4f matrix_set_identity() {
	Mat4x4f m;
	m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
	m.m[0][1] = m.m[0][2] = m.m[0][3] = 0.0f;
	m.m[1][0] = m.m[1][2] = m.m[1][3] = 0.0f;
	m.m[2][0] = m.m[2][1] = m.m[2][3] = 0.0f;
	m.m[3][0] = m.m[3][1] = m.m[3][2] = 0.0f;
	return m;
}

// 平移变换
inline static Mat4x4f matrix_set_translate(float x, float y, float z) {
	Mat4x4f m = matrix_set_identity();
	m.m[0][3] = x;
	m.m[1][3] = y;
	m.m[2][3] = z;
	return m;
}

// 缩放变换
inline static Mat4x4f matrix_set_scale(float x, float y, float z) {
	Mat4x4f m = matrix_set_identity();
	m.m[0][0] = x;
	m.m[1][1] = y;
	m.m[2][2] = z;
	return m;
}



// 旋转变换，围绕 (x, y, z) 矢量旋转 theta 角度
inline static Mat4x4f matrix_set_rotate(float x, float y, float z, float theta) {
	Vec3f n = vector_normalize<3, float>(Vec3f(x, y, z));
	float c = (float)cos(theta);
	float s = (float)sin(theta);
	Mat4x4f m = matrix_set_identity();

	m.m[0][0] = n.x * n.x * (1 - c) + c;
	m.m[0][1] = n.y * n.x * (1 - c) - s * n.z;
	m.m[0][2] = n.z * n.x * (1 - c) + s * n.y;

	m.m[1][0] = n.x * n.y * (1 - c) + s * n.z;
	m.m[1][1] = n.y * n.y * (1 - c) + c;
	m.m[1][2] = n.z * n.y * (1 - c) - s * n.x;

	m.m[2][0] = n.x * n.z * (1 - c) - s * n.y;
	m.m[2][1] = n.y * n.z * (1 - c) + s * n.x;
	m.m[2][2] = n.z * n.z * (1 - c) + c;
	return m;
}
