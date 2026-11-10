#pragma once

#include <cmath>
#include "../Header/Common.h"

class CVector
{
public:
	float x, y, z;
public:
	constexpr CVector() : x(0.0f), y(0.0f), z(0.0f) {}

	constexpr CVector(float f) : x(f), y(f), z(f) {}

	constexpr CVector(float x, float y, float z) : x(x), y(y), z(z) {}

	float SquaredMagnitude()
	{
	    return x * x + y * y + z * z;
	}

	float Magnitude()
	{
	    return std::sqrt(this->SquaredMagnitude());
	}

	void Normalize()
	{
	    float mag = Magnitude();
	    if (mag > 0)
	    {
	        x /= mag;
	        y /= mag;
	        z /= mag;
	    }
	}

	CVector operator-() const {
		return CVector(-x, -y, -z);
	}

	const CVector &operator+=(CVector const &right) {
		x += right.x;
		y += right.y;
		z += right.z;
		return *this;
	}

	const CVector &operator-=(CVector const &right) {
		x -= right.x;
		y -= right.y;
		z -= right.z;
		return *this;
	}

	const CVector &operator*=(float right) {
		x *= right;
		y *= right;
		z *= right;
		return *this;
	}

	const CVector &operator/=(float right) {
		x /= right;
		y /= right;
		z /= right;
		return *this;
	}

	bool operator==(CVector const &right) const {
		return x == right.x && y == right.y && z == right.z;
	}

	bool operator!=(CVector const &right) const {
		return x != right.x || y != right.y || z != right.z;
	}

	bool IsZero(void) const { return x == 0.0f && y == 0.0f && z == 0.0f; }
};
static_assert(sizeof(CVector) == 0xC);

inline CVector operator+(const CVector &left, const CVector &right)
{
	return CVector(left.x + right.x, left.y + right.y, left.z + right.z);
}

inline CVector operator-(const CVector &left, const CVector &right)
{
	return CVector(left.x - right.x, left.y - right.y, left.z - right.z);
}

inline CVector operator*(const CVector &left, float right)
{
	return CVector(left.x * right, left.y * right, left.z * right);
}

inline CVector operator*(float left, const CVector &right)
{
	return CVector(left * right.x, left * right.y, left * right.z);
}

inline CVector operator/(const CVector &left, float right)
{
	return CVector(left.x / right, left.y / right, left.z / right);
}