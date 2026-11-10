#pragma once

#include <cmath>

struct CTiltedVector2
{
    float x;
    float y;

    CTiltedVector2() : x(0.0f), y(0.0f) {}
    CTiltedVector2(float _x, float _y) : x(_x), y(_y) {}
};

struct CTiltedVector3
{
    float x;
    float y;
    float z;

    CTiltedVector3() : x(0.0f), y(0.0f), z(0.0f) {}
    CTiltedVector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    CTiltedVector3 operator+(const CTiltedVector3& b) const { return CTiltedVector3(x + b.x, y + b.y, z + b.z); }
    CTiltedVector3 operator-(const CTiltedVector3& b) const { return CTiltedVector3(x - b.x, y - b.y, z - b.z); }
    CTiltedVector3 operator*(float s) const { return CTiltedVector3(x * s, y * s, z * s); }
    CTiltedVector3 operator-() const { return CTiltedVector3(-x, -y, -z); }

    static float Dot(const CTiltedVector3& a, const CTiltedVector3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static CTiltedVector3 Cross(const CTiltedVector3& a, const CTiltedVector3& b)
    {
        return CTiltedVector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    static CTiltedVector3 Normalize(const CTiltedVector3& v)
    {
        float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (lenSq > 1e-6f)
        {
            float invLen = 1.0f / sqrtf(lenSq);
            return CTiltedVector3(v.x * invLen, v.y * invLen, v.z * invLen);
        }
        return CTiltedVector3(0.0f, 0.0f, 0.0f);
    }
};

struct CTiltedVector4
{
    float x;
    float y;
    float z;
    float w;

    CTiltedVector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    CTiltedVector4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

struct CTiltedMatrix
{
    float m[4][4];

    CTiltedMatrix()
    {
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                m[r][c] = (r == c) ? 1.0f : 0.0f;
            }
        }
    }

    static CTiltedVector4 Transform(const CTiltedVector4& v, const CTiltedMatrix& mat)
    {
        return CTiltedVector4(
            v.x * mat.m[0][0] + v.y * mat.m[1][0] + v.z * mat.m[2][0] + v.w * mat.m[3][0],
            v.x * mat.m[0][1] + v.y * mat.m[1][1] + v.z * mat.m[2][1] + v.w * mat.m[3][1],
            v.x * mat.m[0][2] + v.y * mat.m[1][2] + v.z * mat.m[2][2] + v.w * mat.m[3][2],
            v.x * mat.m[0][3] + v.y * mat.m[1][3] + v.z * mat.m[2][3] + v.w * mat.m[3][3]
        );
    }
};

class CTiltedRadarMath
{
public:
    static constexpr float PI = 3.14159265358979323846f;

    static void BuildRadarViewProj(const CTiltedVector3& cameraPos, const CTiltedVector3& cameraRot, float fov, float nearPlane, float farPlane, float aspect, bool flat2D, CTiltedMatrix& outView, CTiltedMatrix& outProj);
    static bool WorldToScreen(const CTiltedVector3& worldPos, const CTiltedVector3& cameraPos, const CTiltedVector3& cameraRot, float fov, float nearPlane, float farPlane, float screenWidth, float screenHeight, float& screenX, float& screenY, float projectionAspect = 0.0f, bool clipDepth = false);
    static float GetRadarProjectionAspect(bool flat2D = false);
    static bool DirectionToOrbitAngle(const CTiltedVector3& fromPos, const CTiltedVector3& toPos, float yaw, float& outAngle);
    static float DistanceSq2D(const CTiltedVector3& a, const CTiltedVector3& b);
    static float DistanceSq2D(float ax, float ay, float bx, float by);
    static float CalculateDistance2D(const CTiltedVector3& a, const CTiltedVector3& b);
};