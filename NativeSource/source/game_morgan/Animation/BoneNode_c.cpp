#include "BoneNode_c.h"

#include <cmath>

void BoneNode_c::EulerToQuat(RwV3d* angles, RtQuat* orientation)
{
    const float x = ((angles->x * 3.1415927410125732421875f) * 0.0055555556900799274444580078125f) * 0.5f;
    const float y = ((angles->y * 3.1415927410125732421875f) * 0.0055555556900799274444580078125f) * 0.5f;
    const float z = ((angles->z * 3.1415927410125732421875f) * 0.0055555556900799274444580078125f) * 0.5f;
    const float sinX = std::sin(x);
    const float cosX = std::cos(x);
    const float sinY = std::sin(y);
    const float cosY = std::cos(y);
    const float sinZ = std::sin(z);
    const float cosZ = std::cos(z);
    const float cosXcosZ = cosX * cosZ;
    const float cosXsinZ = cosX * sinZ;
    const float sinXsinZ = sinX * sinZ;
    const float sinXcosZ = sinX * cosZ;
    const float qX = std::fma(cosY, sinXcosZ, -(cosXsinZ * sinY));
    const float qY = std::fma(cosY, sinXsinZ, sinY * cosXcosZ);
    const float qZ = std::fma(cosY, cosXsinZ, -(sinXcosZ * sinY));
    const float qW = std::fma(cosY, cosXcosZ, sinY * sinXsinZ);
    const float norm = std::fma(qW, qW, std::fma(qZ, qZ, std::fma(qX, qX, qY * qY)));
    orientation->imag.x = qX / norm;
    orientation->imag.y = qY / norm;
    orientation->imag.z = qZ / norm;
    orientation->real = qW / norm;
}