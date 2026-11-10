#pragma once

#include "CTiltedRadarMath.h"

class CTiltedRadarGeometry
{
private:
    static const float RADAR_OFFSET_X;
    static const float RADAR_OFFSET_Y;

public:
    static void WorldToRadarPos(float worldX, float worldY, float worldZ, float& outRadarX, float& outRadarY, float& outRadarZ);
    static void WorldToRadarPos(float worldX, float worldY, CTiltedVector3& outRadarPos);
    static bool WorldToCircleScreen(const CTiltedVector3& worldPos, const CTiltedVector3& cameraPos, const CTiltedVector3& cameraRot, float fov, float nearPlane, float farPlane, float rtWidth, float rtHeight, float sizeX, float sizeY, float centerX, float centerY, float& outCircleX, float& outCircleY, float projectionAspect = 0.0f);
    static void ClampToOrbit(float circleScreenX, float circleScreenY, float centerX, float centerY, float halfX, float halfY, float& outX, float& outY, bool useSquare = false);
    static void PointOnOrbitEdge(float centerX, float centerY, float halfX, float halfY, float cosA, float sinA, bool useSquare, float& outX, float& outY);
    static bool IsInsideOrbit(float x, float y, float centerX, float centerY, float halfX, float halfY, bool useSquare = false);
};