#include "CTiltedRadarGeometry.h"

const float CTiltedRadarGeometry::RADAR_OFFSET_X = 3000.0f;
const float CTiltedRadarGeometry::RADAR_OFFSET_Y = -3000.0f;

void CTiltedRadarGeometry::WorldToRadarPos(float worldX, float worldY, float worldZ, float& outRadarX, float& outRadarY, float& outRadarZ)
{
    outRadarX = worldX + CTiltedRadarGeometry::RADAR_OFFSET_X;
    outRadarY = worldY + CTiltedRadarGeometry::RADAR_OFFSET_Y;
    outRadarZ = worldZ;
}

void CTiltedRadarGeometry::WorldToRadarPos(float worldX, float worldY, CTiltedVector3& outRadarPos)
{
    outRadarPos.x = worldX + CTiltedRadarGeometry::RADAR_OFFSET_X;
    outRadarPos.y = worldY + CTiltedRadarGeometry::RADAR_OFFSET_Y;
    outRadarPos.z = 0.1f;
}

bool CTiltedRadarGeometry::WorldToCircleScreen(const CTiltedVector3& worldPos, const CTiltedVector3& cameraPos, const CTiltedVector3& cameraRot, float fov, float nearPlane, float farPlane, float rtWidth, float rtHeight, float sizeX, float sizeY, float centerX, float centerY, float& outCircleX, float& outCircleY, float projectionAspect)
{
    float screenX = 0.0f;
    float screenY = 0.0f;
    if (!CTiltedRadarMath::WorldToScreen(worldPos, cameraPos, cameraRot, fov, nearPlane, farPlane, rtWidth, rtHeight, screenX, screenY, projectionAspect, false)) {
        return false;
    }

    float nx = screenX / rtWidth;
    float ny = screenY / rtHeight;
    outCircleX = centerX + (nx - 0.5f) * sizeX;
    outCircleY = centerY + (ny - 0.5f) * sizeY;
    return true;
}

void CTiltedRadarGeometry::ClampToOrbit(float circleScreenX, float circleScreenY, float centerX, float centerY, float halfX, float halfY, float& outX, float& outY, bool useSquare)
{
    float dx = circleScreenX - centerX;
    float dy = circleScreenY - centerY;

    if (useSquare)
    {
        float normX = (halfX > 0.01f) ? (fabsf(dx) / halfX) : 0.0f;
        float normY = (halfY > 0.01f) ? (fabsf(dy) / halfY) : 0.0f;
        float maxNorm = (normX > normY) ? normX : normY;
        if (maxNorm > 1.0f && maxNorm > 0.01f)
        {
            float scale = 1.0f / maxNorm;
            dx *= scale;
            dy *= scale;
        }

        outX = centerX + dx;
        outY = centerY + dy;
    }
    else
    {
        float r = (halfX < halfY) ? halfX : halfY;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.01f)
        {
            dx /= len;
            dy /= len;
        }

        outX = centerX + dx * r;
        outY = centerY + dy * r;
    }
}

void CTiltedRadarGeometry::PointOnOrbitEdge(float centerX, float centerY, float halfX, float halfY, float cosA, float sinA, bool useSquare, float& outX, float& outY)
{
    if (useSquare)
    {
        float tx = (fabsf(cosA) > 1e-6f) ? (halfX / fabsf(cosA)) : 1e6f;
        float ty = (fabsf(sinA) > 1e-6f) ? (halfY / fabsf(sinA)) : 1e6f;
        float t = (tx < ty) ? tx : ty;
        outX = centerX + cosA * t;
        outY = centerY + sinA * t;
    }
    else
    {
        float r = (halfX < halfY) ? halfX : halfY;
        outX = centerX + cosA * r;
        outY = centerY + sinA * r;
    }
}

bool CTiltedRadarGeometry::IsInsideOrbit(float x, float y, float centerX, float centerY, float halfX, float halfY, bool useSquare)
{
    float dx = x - centerX;
    float dy = y - centerY;
    if (useSquare)
    {
        return (fabsf(dx) <= halfX && fabsf(dy) <= halfY);
    }

    float r = (halfX < halfY) ? halfX : halfY;
    return (dx * dx + dy * dy) <= (r * r);
}