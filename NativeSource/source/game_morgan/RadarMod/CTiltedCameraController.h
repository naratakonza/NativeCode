#pragma once

#include "CTiltedRadarMath.h"

class CTiltedCameraController
{
public:
    struct CameraState
    {
        float height;
        float pitch;
        float yaw;
        float posX;
        float posY;
        float offsetX;
        float offsetY;
        float fov;
    };

    CTiltedCameraController();

    void Update(float playerX, float playerY, float yaw, bool inVehicle, bool isPlane, bool isHeli, float speedKMH);
    void GetCachedCalculations(float& offsetWorldX, float& offsetWorldY, CTiltedVector3& cameraPos, CTiltedVector3& cameraRot, float cameraHeight = 0.0f);

    void SetState(const CameraState& state);

    const CameraState& GetState() const {
        return m_state;
    }

private:
    struct CameraTarget
    {
        float height;
        float offsetX;
        float offsetY;
        float fov;
        float pitch;
    };

    static const float DEFAULT_CAMERA_HEIGHT;
    static const float DEFAULT_FOV_DEGREES;
    static const float DEFAULT_CAMERA_OFFSET_Y;
    static const float DEFAULT_CAMERA_PITCH;

    static const float VEHICLE_MIN_HEIGHT;
    static const float VEHICLE_MIN_OFFSET_Y;
    static const float VEHICLE_MIN_PITCH;

    static const float SPEED_MAX_HEIGHT;
    static const float SPEED_MAX_OFFSET_Y;
    static const float SPEED_MAX_PITCH;
    static const float SPEED_MAX_FOV_RAD;
    static const float DEFAULT_FOV_RAD;
    static const float FOV_DELTA_RAD;

    static const float CAMERA_HEIGHT_PLANE;
    static const float CAMERA_HEIGHT_HELICOPTER;
    static const float CAMERA_OFFSET_Y_PLANE;
    static const float CAMERA_OFFSET_Y_HELICOPTER;
    static const float CAMERA_PITCH_PLANE;
    static const float CAMERA_PITCH_HELICOPTER;

    void UpdateInterpolation();

    CameraState m_state;
    CameraTarget m_target;

    struct CalcCache
    {
        float cachedInvertedYaw;
        float cachedSinYaw;
        float cachedCosYaw;
        float cachedTanHalfFov;
        CTiltedVector3 cachedCameraPos;
        CTiltedVector3 cachedCameraRot;
        bool isValid;
    } m_cache;
};