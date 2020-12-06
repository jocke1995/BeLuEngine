#ifndef ENGINEMATH_H
#define ENGINEMATH_H

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <algorithm>

#define PI           3.14159265358979323846f
#define EPSILON		  0.000001

typedef DirectX::XMMATRIX float4x4;
typedef DirectX::XMFLOAT3X3 float3x3;

#pragma region XMFLOAT3_OPERATOR_OVERLOADS
inline DirectX::XMFLOAT3 operator*(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
{
	return { a.x * b.x, a.y * b.y, a.z * b.z };
}

inline DirectX::XMFLOAT3 operator*(DirectX::XMFLOAT3 a, float b)
{
	return { a.x * b, a.y * b, a.z * b };
}

inline DirectX::XMFLOAT3 operator/(DirectX::XMFLOAT3 a, float b)
{
	return { a.x / b, a.y / b, a.z / b };
}

inline DirectX::XMFLOAT3 operator+(DirectX::XMFLOAT3 a, float b)
{
	return { a.x + b, a.y + b, a.z + b };
}

inline DirectX::XMFLOAT3 operator-(DirectX::XMFLOAT3 a, float b)
{
	return { a.x - b, a.y - b, a.z - b };
}

inline DirectX::XMFLOAT3 operator+(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline DirectX::XMFLOAT3 operator-(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}
#pragma endregion XMFLOAT3_OPERATOR_OVERLOADS
#pragma region floatStructs
typedef union
{
	struct { float x; float y; float z; float w; };
	struct { float r; float g; float b; float a; };
} float4;

typedef union float3
{
	struct { float x; float y; float z; };
	struct { float r; float g; float b; };

	float length() const
	{
		return sqrt(x * x + y * y + z * z);
	};

	void normalize()
	{
		float length = this->length();
		if (length > EPSILON)
		{
			x /= length;
			y /= length;
			z /= length;
		}
	};

	void operator /= (float denom)
	{
		x /= denom;
		y /= denom;
		z /= denom;
	};

	void operator *= (float factor)
	{
		x *= factor;
		y *= factor;
		z *= factor;
	};

	float3 operator + (const float3& other) const
	{
		return {x + other.x, y + other.y, z + other.z};
	};

	float3 operator - (const float3& other) const
	{
		return { x - other.x, y - other.y, z - other.z };
	};

	float3 operator * (float factor)
	{
		return { x * factor, y * factor, z * factor };
	};

	bool operator == (float3 other)
	{
		return (x == other.x && y == other.y && z == other.z);
	}

	bool operator != (float3 other) const
	{
		return (x != other.x || y != other.y || z != other.z);
	}

	float3 cross(const float3& that) const
	{
		return
		{
			this->y * that.z - this->z * that.y,
			this->z * that.x - this->x * that.z,
			this->x * that.y - this->y * that.x,
		};
	};

	float dot(const float3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}
	
	float angle(const float3& other) const
	{
		return acosf(std::min<float>(std::max<float>(dot(other) / (length() * other.length()), -1.0f), 1.0f));
	}

} float3;

typedef union float2
{
	struct { float x; float y; };
	struct { float u; float v; };

	float2 operator +(const float2& other) const
	{
		return { x + other.x, y + other.y };
	}

	float2 operator -(const float2& other) const
	{
		return { x - other.x, y - other.y };
	}
} float2;

#pragma endregion floatStructs
#pragma region intStructs
typedef union
{
	struct { int x; int y; };
	struct { int r; int g; };
} int2;

typedef union
{
	struct { int x; int y; int z; };
	struct { int r; int g; int b; };
} int3;


typedef union
{
	struct { int x; int y; int z; int w; };
	struct { int r; int g; int b; int a; };
} int4;
#pragma endregion intStructs

struct Quaternion
{
	double x, y, z, w;
};

// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
inline Quaternion ToQuaternion(double yaw, double pitch, double roll) // yaw (Z), pitch (Y), roll (X)
{
	// Abbreviations for the various angular functions
	double cy = cos(yaw * 0.5);
	double sy = sin(yaw * 0.5);
	double cp = cos(pitch * 0.5);
	double sp = sin(pitch * 0.5);
	double cr = cos(roll * 0.5);
	double sr = sin(roll * 0.5);

	Quaternion q;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;
	q.w = cr * cp * cy + sr * sp * sy;

	return q;
}

class EngineMath
{
public:
	static inline float convertToRadians(float degrees)
	{
		return degrees * (PI / 180.0f);
	}

	static inline float convertToDegrees(float radians)
	{
		return radians * (180.0f / PI);
	}

	static inline int convertToWholeDegrees(float radians)
	{
		return static_cast<int>(radians * (180.0f / PI));
	}
private:
	EngineMath() {};
};

#endif