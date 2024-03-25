#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Camera.hpp"
#include <Config.hpp>
#include <math.h>


class RenderUtils
{
public:
	// 将裁剪空间中的坐标转换到屏幕空间中的坐标
	static float4 ClipPositionToScreenPosition(float4 clipPos, float3& ndcPos)
	{
		// 将裁剪空间中的坐标转换为NDC空间中的坐标
		ndcPos = clipPos.xyz / clipPos.w;
		// 将NDC空间中的坐标转换为屏幕空间中的坐标, 屏幕坐标是从0开始
		float4 screenPos = float4(
			(ndcPos.x + 1.0f) * 0.5f * (Config::kScreenWidth - 1),
			(ndcPos.y + 1.0f) * 0.5f * (Config::kScreenHeight - 1),
			// ndcPos.z * (f - n) / 2 + (f + n) / 2,
			ndcPos.z * 0.5f + 0.5f,
			// w透视矫正系数
			clipPos.w
		);
		return screenPos;
	}

	static float3 Rotate2Forward(float3 rotate)
	{
		return float3();
	}


	static Mat4x4f CreateMatrixVP(Camera camera)
	{
		float3 position = camera.Position;
		float3 forward = Rotate2Forward(camera.Rotation);
		float3 up(0, 1, 0);
		// 创建一个视图变换矩阵
		Mat4x4f viewMatrix = CreateViewMatrix(position, forward, up);
		// float4x4 viewMatrix = Matrix4x4.TRS(camera.transform.position, camera.transform.rotation, Vector3.one).inverse;

		// 创建一个透视投影矩阵
		Mat4x4f projectionMatrix = Perspective(camera.Near, camera.Far, camera.Fov, camera.Aspect);

		// 将视图矩阵和投影矩阵相乘，得到最终的视图投影矩阵
		//return projectionMatrix * viewMatrix;
		return projectionMatrix * viewMatrix;
	}

	static float4x4 CreateViewMatrix(float3 position, float3 forward, float3 up)
	{
		// 计算相机的右向量
		float3 right = vector_normalize(vector_cross(up, forward));

		// 计算相机的上向量
		up = vector_normalize(vector_cross(forward, right));

		// 创建一个变换矩阵，将相机的位置和方向转换为一个矩阵
		float4x4 viewMatrix = {
			float4(right.x, up.x, forward.x, 0),
			float4(right.y, up.y, forward.y, 0),
			float4(right.z, up.z, forward.z, 0),
			float4(-dot(right, position), -dot(up, position), -dot(forward, position), 1)
		};
		return viewMatrix;
	}

	static float4x4 Perspective(float near, float far, float fov, float aspect)
	{
		float rad = fov * PI / 180.f;
		float tanHalfFov = tan(rad / 2);
		float fovY = 1 / tanHalfFov;
		float fovX = fovY / aspect;
		float4x4 perspectiveMatrix{
			float4(fovX, 0, 0, 0),
			float4(0, fovY, 0, 0),
			float4(0, 0, (far + near) / (far - near), 1),
			float4(0, 0, -(2 * far * near) / (far - near), 0)
		};
		return perspectiveMatrix;
	}

	/// <summary>
	/// 高效的重心坐标算法
	/// (https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling)
	static float3 BarycentricCoordinate(float2 P, float2 v0, float2 v1, float2 v2)
	{
		float2 v2v0 = v2 - v0;
		float2 v1v0 = v1 - v0;
		float2 v0P = v0 - P;
		float3 u = vector_cross(float3(v2v0.x, v1v0.x, v0P.x), float3(v2v0.y, v1v0.y, v0P.y));
		// float3 u = vector_cross(float3(v2.x - v0.x, v1.x - v0.x, v0.x - P.x), float3(v2.y - v0.y, v1.y - v0.y, v0.y - P.y));
		if (abs(u.z) < 1) return float3(-1, 1, 1);
		return float3(1 - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	}

	inline static const float NegativeInfinity = -0.000001f;

	///  重心坐标(https://zhuanlan.zhihu.com/p/538468807)
	static float3 BarycentricCoordinate2(float2 p, float3 v0, float3 v1, float3 v2)
	{
		// 计算三角形三个边的向量(左手坐标系，顺时针为正，逆时针为负)
		float3 v0v1(v1.xy - v0.xy, 0);
		float3 v1v2(v2.xy - v1.xy, 0);
		float3 v2v0(v0.xy - v2.xy, 0);
		// 计算点p到三角形三个顶点的向量
		float3 v0p(p - v0.xy, 0);
		float3 v1p(p - v1.xy, 0);
		float3 v2p(p - v2.xy, 0);

		// 计算三角形的法向量，用来判断三角形的正反面
		float3 normal = vector_cross(v2v0, v0v1);
		// 大三角形面积，这里没有除以2，因为后面计算的时候会相互抵消
		float area = abs(normal.z);
		// 方向向量
		normal = vector_normalize(normal);

		// 计算三角形的面积：
		// 叉乘可以用来计算两个向量所在平行四边形的面积，因为叉乘的结果是一个向量，
		// 将这个向量与单位法向量进行点乘，可以得到一个有向的面积。
		// 小三角形面积
		// float area0 = dot(cross(v1v2, v1p), normal);
		// float area1 = dot(cross(v2v0, v2p), normal);
		// float area2 = dot(cross(v0v1, v0p), normal);

		// 又因为所有的点z都为0，所以z就是向量的模长，也就是面积，所以可以进一步简化为：
		float area0 = vector_cross(v1v2, v1p).z * normal.z;
		float area1 = vector_cross(v2v0, v2p).z * normal.z;
		float area2 = vector_cross(v0v1, v0p).z * normal.z;


		return float3(area0 / area, area1 / area, area2 / area);
	}




	// 四元数转换为旋转矩阵(https://blog.csdn.net/silangquan/article/details/39008903)
	static float4x4 QuaternionToMatrix(float4 rotation)
	{
		float x = rotation.x;
		float y = rotation.y;
		float z = rotation.z;
		float w = rotation.w;
		// 模长 
		float n = 1.0f / sqrt(x * x + y * y + z * z + w * w);
		// 归一化,将四元数的四个分量除以它们的模长
		x *= n;
		y *= n;
		z *= n;
		w *= n;

		// R = | 1 - 2y^2 - 2z^2   2xy - 2zw       2xz + 2yw       0 |
		//     | 2xy + 2zw         1 - 2x^2 - 2z^2   2yz - 2xw       0 |
		//     | 2xz - 2yw         2yz + 2xw        1 - 2x^2 - 2y^2  0 |
		//     | 0                 0                0               1 |
		float4x4 matrix = createMatrix4x4<float>(
			1.0f - 2.0f * y * y - 2.0f * z * z, 2.0f * x * y - 2.0f * z * w, 2.0f * x * z + 2.0f * y * w, 0.0f,
			2.0f * x * y + 2.0f * z * w, 1.0f - 2.0f * x * x - 2.0f * z * z, 2.0f * y * z - 2.0f * x * w, 0.0f,
			2.0f * x * z - 2.0f * y * w, 2.0f * y * z + 2.0f * x * w, 1.0f - 2.0f * x * x - 2.0f * y * y, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);
		return matrix;
	}

	enum BlendMode
	{
		None,                   // 不进行混合
		AlphaBlend,             // 标准的透明混合，使用源颜色的alpha值来控制源颜色和目标颜色的混合比例
		Additive,               // 加法混合，将源颜色和目标颜色相加
		Subtractive,            // 减法混合，将源颜色和目标颜色相减
		PremultipliedAlpha,     // 预乘alpha混合，先将源颜色的RGB值乘以alpha值，再进行标准的透明混合
		Multiply,               // 乘法混合，将源颜色和目标颜色相乘
		Screen,                 // 屏幕混合，将源颜色和目标颜色的补色相乘，再取补色
		Overlay,                // 叠加混合，根据源颜色的亮度值来决定是乘以目标颜色还是相加目标颜色
		Darken,                 // 取暗混合，将源颜色和目标颜色中较暗的那个作为混合结果
		Lighten,                // 取亮混合，将源颜色和目标颜色中较亮的那个作为混合结果
		ColorDodge,             // 颜色减淡混合，将源颜色和目标颜色相除
		ColorBurn,              // 颜色加深混合，将源颜色的补色和目标颜色的补色相除，再取补色
		SoftLight,              // 柔光混合，根据源颜色的亮度值来决定是调暗还是调亮目标颜色，类似于叠加混合
		HardLight,              // 强光混合，根据源颜色的亮度值来决定是调暗还是调亮目标颜色，类似于叠加混合
		Difference,             // 差值混合，将源颜色和目标颜色相减，再取绝对值
		Exclusion,              // 排除混合，将源颜色和目标颜色相加，再减去两者的乘积
		HSLHue,                 // 色相混合
		HSLSaturation,          // 饱和度混合
		HSLColor,               // 颜色混合
		HSLLuminosity,          // 亮度混合
	};

	static float4 BlendColors(float4 srcColor, float4 dstColor, BlendMode blendMode)
	{
		switch (blendMode)
		{
		case None:
			return srcColor;
		case AlphaBlend:
			return srcColor.w * srcColor + (1 - srcColor.w) * dstColor;
		case Additive:
			return srcColor + dstColor;
		case Subtractive:
			return srcColor - dstColor;
		case PremultipliedAlpha:
			return srcColor + (1 - srcColor.w) * dstColor;
		case Multiply:
			return srcColor * dstColor;
		case Screen:
			return srcColor + dstColor - srcColor * dstColor;
		//case Overlay:
		//	return dstColor.w < 0.5f ? 2 * srcColor * dstColor : 1 - 2 * (1 - srcColor) * (1 - dstColor);
		//case Darken:
		//	return min(srcColor, dstColor);
		//case Lighten:
		//	return max(srcColor, dstColor);
		//case ColorDodge:
		//	return dstColor.Equals(float4(0)) ? dstColor : min(float4(1), srcColor / (float4(1) - dstColor));
		//case ColorBurn:
		//	return dstColor.Equals(float4(1)) ? dstColor : max(float4(0), (float4(1) - srcColor) / dstColor);
		//case SoftLight:
		//	return dstColor * (2 * srcColor + srcColor * srcColor - 2 * srcColor * dstColor + 2 * dstColor - 2 * dstColor * dstColor);
		//case HardLight:
		//	return BlendColors(dstColor, srcColor, BlendMode.Overlay);
		//case Difference:
		//	return abs(srcColor - dstColor);
		//case Exclusion:
		//	return srcColor + dstColor - 2 * srcColor * dstColor;
		default:
			return srcColor;
		}
	}

	enum class ClipPlane
	{
		W_PLANE,
		X_RIGHT,
		X_LEFT,
		Y_TOP,
		Y_BOTTOM,
		Z_NEAR,
		Z_FAR
	};


	static float get_intersect_ratio(float4 prev, float4 curv, ClipPlane c_plane)
	{
		switch (c_plane)
		{
		case ClipPlane::W_PLANE:
			return (prev.w - EPSILON) / (prev.w - curv.w);
		case ClipPlane::X_RIGHT:
			return (prev.w - prev.x) / ((prev.w - prev.x) - (curv.w - curv.x));
		case ClipPlane::X_LEFT:
			return (prev.w + prev.x) / ((prev.w + prev.x) - (curv.w + curv.x));
		case ClipPlane::Y_TOP:
			return (prev.w - prev.y) / ((prev.w - prev.y) - (curv.w - curv.y));
		case ClipPlane::Y_BOTTOM:
			return (prev.w + prev.y) / ((prev.w + prev.y) - (curv.w + curv.y));
		case ClipPlane::Z_NEAR:
			return (prev.w - prev.z) / ((prev.w - prev.z) - (curv.w - curv.z));
		case ClipPlane::Z_FAR:
			return (prev.w + prev.z) / ((prev.w + prev.z) - (curv.w + curv.z));
		default:
			return 0;
		}
	}

	static bool IsVisible(float4 posCS)
	{
		posCS = posCS / posCS.w;
		if (posCS.x < -1 || posCS.x > 1 || posCS.y < -1 || posCS.y > 1 || posCS.z < -1 || posCS.z > 1)
		{
			return false;
		}
		return true;
	}

	static bool IsInsidePlane(ClipPlane c_plane, float4 vertex)
	{
		switch (c_plane)
		{
		case ClipPlane::W_PLANE:
			return vertex.w > EPSILON;
		case ClipPlane::X_RIGHT:
			return vertex.x / vertex.w <= 1;
		case ClipPlane::X_LEFT:
			return vertex.x / vertex.w >= -1;
		case ClipPlane::Y_TOP:
			return vertex.y / vertex.w <= 1;
		case ClipPlane::Y_BOTTOM:
			return vertex.y / vertex.w >= -1;
		case ClipPlane::Z_NEAR:
			return vertex.z / vertex.w <= 1;
		case ClipPlane::Z_FAR:
			return vertex.z / vertex.w >= -1;
		default:
			return false;
		}
	}

	static float3 RotationToDirection(float rot_x, float rot_y, float rot_z, float3 src_dir)
	{
		float3x3 init = createMatrix3x3<float>(
			1, 0, 0,
			0, 1, 0,
			0, 0, -1);

		float rx = rot_x * PI / 180.f;
		float3x3 rotateX = createMatrix3x3<float>
			(1, 0, 0,
				0, cos(rx), -sin(rx),
				0, sin(rx), cos(rx));

		float ry = rot_y * PI / 180.f;
		float3x3 rotateY = createMatrix3x3<float>
			(cos(ry), 0, sin(ry),
				0, 1, 0,
				-sin(ry), 0, cos(ry));

		float rz = rot_z * PI / 180.f;
		float3x3 rotateZ = createMatrix3x3<float>
			(cos(rz), sin(rz), 0,
				-sin(rz), cos(rz), 0,
				0, 0, 1);

		float3x3 rotate = rotateZ * rotateX * rotateY;
		float3x3 rm = rotate * init;
		float3 dir = src_dir * rm;
		return dir;
	}


};