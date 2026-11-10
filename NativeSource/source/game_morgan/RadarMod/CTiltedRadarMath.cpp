#include "CTiltedRadarMath.h"

void CTiltedRadarMath::BuildRadarViewProj(const CTiltedVector3& cameraPos, const CTiltedVector3& cameraRot, float fov, float nearPlane, float farPlane, float aspect, bool flat2D, CTiltedMatrix& outView, CTiltedMatrix& outProj)
{
    CTiltedVector3 xaxis, yaxis, zaxis, viewPos;

    if (flat2D)
    {
        const float yaw = cameraRot.z;
        const float sy = sinf(yaw);
        const float cy = cosf(yaw);
        viewPos = cameraPos;
        zaxis = CTiltedVector3(0.0f, 0.0f, -1.0f);
        CTiltedVector3 up(-sy, cy, 0.0f);
        CTiltedVector3 negUp(-up.x, -up.y, -up.z);
        xaxis = CTiltedVector3::Normalize(CTiltedVector3::Cross(negUp, zaxis));
        yaxis = CTiltedVector3::Cross(xaxis, zaxis);
    }
    else
    {
        const float pitch = cameraRot.x;
        const float yaw = cameraRot.z;
        const float cp = cosf(pitch);
        const float sp = sinf(pitch);
        const float cy = cosf(yaw);
        const float sy = sinf(yaw);

        CTiltedMatrix camWorld;
        camWorld.m[0][0] = cy;
        camWorld.m[0][1] = sy;
        camWorld.m[0][2] = 0.0f;
        camWorld.m[0][3] = 0.0f;
        camWorld.m[1][0] = -cp * sy;
        camWorld.m[1][1] = cy * cp;
        camWorld.m[1][2] = sp;
        camWorld.m[1][3] = 0.0f;
        camWorld.m[2][0] = sy * sp;
        camWorld.m[2][1] = -cy * sp;
        camWorld.m[2][2] = cp;
        camWorld.m[2][3] = 0.0f;
        camWorld.m[3][0] = cameraPos.x;
        camWorld.m[3][1] = cameraPos.y;
        camWorld.m[3][2] = cameraPos.z;
        camWorld.m[3][3] = 1.0f;

        CTiltedVector3 forwardVec(camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2]);
        CTiltedVector3 downVec(0.0f, 0.0f, -1.0f);
        float dotProduct = CTiltedVector3::Dot(downVec, forwardVec);
        if (dotProduct > 1.0f) dotProduct = 1.0f;
        if (dotProduct < -1.0f) dotProduct = -1.0f;
        const float rotOff = 600.0f * acosf(dotProduct) / (0.5f * CTiltedRadarMath::PI);

        CTiltedVector3 offX(
            camWorld.m[0][0] + camWorld.m[1][0] - rotOff * camWorld.m[2][0],
            camWorld.m[0][1] + camWorld.m[1][1] - rotOff * camWorld.m[2][1],
            camWorld.m[0][2] + camWorld.m[1][2] - rotOff * camWorld.m[2][2]
        );

        viewPos = CTiltedVector3(camWorld.m[3][0] + offX.x, camWorld.m[3][1] + offX.y, camWorld.m[3][2] + offX.z);
        zaxis = CTiltedVector3::Normalize(forwardVec);

        CTiltedVector3 upVec(camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2]);
        CTiltedVector3 negUpVec(-upVec.x, -upVec.y, -upVec.z);
        xaxis = CTiltedVector3::Normalize(CTiltedVector3::Cross(negUpVec, zaxis));
        yaxis = CTiltedVector3::Cross(xaxis, zaxis);
    }

    outView.m[0][0] = xaxis.x;
    outView.m[0][1] = yaxis.x;
    outView.m[0][2] = zaxis.x;
    outView.m[0][3] = 0.0f;
    outView.m[1][0] = xaxis.y;
    outView.m[1][1] = yaxis.y;
    outView.m[1][2] = zaxis.y;
    outView.m[1][3] = 0.0f;
    outView.m[2][0] = xaxis.z;
    outView.m[2][1] = yaxis.z;
    outView.m[2][2] = zaxis.z;
    outView.m[2][3] = 0.0f;
    outView.m[3][0] = -CTiltedVector3::Dot(xaxis, viewPos);
    outView.m[3][1] = -CTiltedVector3::Dot(yaxis, viewPos);
    outView.m[3][2] = -CTiltedVector3::Dot(zaxis, viewPos);
    outView.m[3][3] = 1.0f;

    const float useAspect = (aspect > 0.0f) ? aspect : 1.0f;
    const float w = 1.0f / tanf(fov * 0.5f);
    const float h = w / useAspect;
    const float Q = farPlane / (farPlane - nearPlane);

    outProj = CTiltedMatrix();
    outProj.m[0][0] = w;
    outProj.m[1][1] = h;
    outProj.m[2][2] = Q;
    outProj.m[2][3] = 1.0f;
    outProj.m[3][2] = -Q * nearPlane;
    outProj.m[3][3] = 0.0f;
}

bool CTiltedRadarMath::WorldToScreen(const CTiltedVector3& worldPos, const CTiltedVector3& cameraPos, const CTiltedVector3& cameraRot, float fov, float nearPlane, float farPlane, float screenWidth, float screenHeight, float& screenX, float& screenY, float projectionAspect, bool clipDepth)
{
    float aspect = (projectionAspect > 0.0f) ? projectionAspect : (screenHeight / screenWidth);

    static CTiltedVector3 s_lastCamPos(-999999.0f, -999999.0f, -999999.0f);
    static CTiltedVector3 s_lastCamRot(-999999.0f, -999999.0f, -999999.0f);
    static float s_lastFov = -1.0f;
    static float s_lastNear = -1.0f;
    static float s_lastFar = -1.0f;
    static float s_lastAspect = -1.0f;
    static CTiltedMatrix s_cachedVP;

    if (cameraPos.x != s_lastCamPos.x || cameraPos.y != s_lastCamPos.y || cameraPos.z != s_lastCamPos.z || cameraRot.x != s_lastCamRot.x || cameraRot.y != s_lastCamRot.y || cameraRot.z != s_lastCamRot.z || fov != s_lastFov || nearPlane != s_lastNear || farPlane != s_lastFar || aspect != s_lastAspect)
    {
        s_lastCamPos = cameraPos;
        s_lastCamRot = cameraRot;
        s_lastFov = fov;
        s_lastNear = nearPlane;
        s_lastFar = farPlane;
        s_lastAspect = aspect;

        CTiltedMatrix view, proj;
        CTiltedRadarMath::BuildRadarViewProj(cameraPos, cameraRot, fov, nearPlane, farPlane, aspect, false, view, proj);

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                s_cachedVP.m[i][j] = view.m[i][0] * proj.m[0][j] + view.m[i][1] * proj.m[1][j] + view.m[i][2] * proj.m[2][j] + view.m[i][3] * proj.m[3][j];
            }
        }
    }

    CTiltedVector4 worldPos4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
    CTiltedVector4 clipPos4 = CTiltedMatrix::Transform(worldPos4, s_cachedVP);

    if (clipPos4.w <= 0.0f) {
        return false;
    }

    if (clipDepth && (clipPos4.z < 0.0f || clipPos4.z > clipPos4.w)) {
        return false;
    }

    float invW = 1.0f / clipPos4.w;
    float ndcX = clipPos4.x * invW;
    float ndcY = clipPos4.y * invW;

    screenX = (ndcX + 1.0f) * 0.5f * screenWidth;
    screenY = (1.0f - ndcY) * 0.5f * screenHeight;

    return true;
}

float CTiltedRadarMath::GetRadarProjectionAspect(bool flat2D)
{
    return flat2D ? 1.0f : (1080.0f / 1920.0f);
}

bool CTiltedRadarMath::DirectionToOrbitAngle(const CTiltedVector3& fromPos, const CTiltedVector3& toPos, float yaw, float& outAngle)
{
    float dx = toPos.x - fromPos.x;
    float dy = toPos.y - fromPos.y;
    float lenSq = dx * dx + dy * dy;
    if (lenSq < 0.0001f) {
        return false;
    }

    float worldAngle = atan2f(dy, dx);
    outAngle = -worldAngle - yaw;
    return true;
}

float CTiltedRadarMath::DistanceSq2D(const CTiltedVector3& a, const CTiltedVector3& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

float CTiltedRadarMath::DistanceSq2D(float ax, float ay, float bx, float by)
{
    float dx = ax - bx;
    float dy = ay - by;
    return dx * dx + dy * dy;
}

float CTiltedRadarMath::CalculateDistance2D(const CTiltedVector3& a, const CTiltedVector3& b)
{
    return sqrtf(CTiltedRadarMath::DistanceSq2D(a, b));
}