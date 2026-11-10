#include "CTiltedCameraController.h"

const float CTiltedCameraController::DEFAULT_CAMERA_HEIGHT = 475.0f;
const float CTiltedCameraController::DEFAULT_FOV_DEGREES = 70.0f;
const float CTiltedCameraController::DEFAULT_CAMERA_OFFSET_Y = -48.0f;
const float CTiltedCameraController::DEFAULT_CAMERA_PITCH = -29.5f;

const float CTiltedCameraController::VEHICLE_MIN_HEIGHT = 485.0f;
const float CTiltedCameraController::VEHICLE_MIN_OFFSET_Y = -38.0f;
const float CTiltedCameraController::VEHICLE_MIN_PITCH = -29.5f;

const float CTiltedCameraController::SPEED_MAX_HEIGHT = 510.0f;
const float CTiltedCameraController::SPEED_MAX_OFFSET_Y = -10.0f;
const float CTiltedCameraController::SPEED_MAX_PITCH = -29.5f;
const float CTiltedCameraController::SPEED_MAX_FOV_RAD = 72.0f * (CTiltedRadarMath::PI / 180.0f);
const float CTiltedCameraController::DEFAULT_FOV_RAD = 70.0f * (CTiltedRadarMath::PI / 180.0f);
const float CTiltedCameraController::FOV_DELTA_RAD = CTiltedCameraController::SPEED_MAX_FOV_RAD - CTiltedCameraController::DEFAULT_FOV_RAD;

const float CTiltedCameraController::CAMERA_HEIGHT_PLANE = 520.0f;
const float CTiltedCameraController::CAMERA_HEIGHT_HELICOPTER = 500.0f;
const float CTiltedCameraController::CAMERA_OFFSET_Y_PLANE = 0.0f;
const float CTiltedCameraController::CAMERA_OFFSET_Y_HELICOPTER = -20.0f;
const float CTiltedCameraController::CAMERA_PITCH_PLANE = -29.5f;
const float CTiltedCameraController::CAMERA_PITCH_HELICOPTER = -29.5f;

CTiltedCameraController::CTiltedCameraController()
{
    m_state.height = CTiltedCameraController::DEFAULT_CAMERA_HEIGHT;
    m_state.pitch = CTiltedCameraController::DEFAULT_CAMERA_PITCH * (CTiltedRadarMath::PI / 180.0f);
    m_state.yaw = 0.0f;
    m_state.posX = 0.0f;
    m_state.posY = 0.0f;
    m_state.offsetX = 0.0f;
    m_state.offsetY = CTiltedCameraController::DEFAULT_CAMERA_OFFSET_Y;
    m_state.fov = CTiltedCameraController::DEFAULT_FOV_RAD;

    m_target.height = CTiltedCameraController::DEFAULT_CAMERA_HEIGHT;
    m_target.offsetX = 0.0f;
    m_target.offsetY = CTiltedCameraController::DEFAULT_CAMERA_OFFSET_Y;
    m_target.fov = CTiltedCameraController::DEFAULT_FOV_RAD;
    m_target.pitch = CTiltedCameraController::DEFAULT_CAMERA_PITCH * (CTiltedRadarMath::PI / 180.0f);

    m_cache.cachedInvertedYaw = 0.0f;
    m_cache.cachedSinYaw = 0.0f;
    m_cache.cachedCosYaw = 1.0f;
    m_cache.cachedTanHalfFov = tanf(CTiltedCameraController::DEFAULT_FOV_RAD * 0.5f);
    m_cache.isValid = false;
}

void CTiltedCameraController::Update(float playerX, float playerY, float yaw, bool inVehicle, bool isPlane, bool isHeli, float speedKMH)
{
    m_state.posX = playerX + 3000.0f;
    m_state.posY = playerY - 3000.0f;
    m_state.yaw = yaw;

    m_target.offsetX = 0.0f;

    if (!inVehicle)
    {
        m_target.height = CTiltedCameraController::DEFAULT_CAMERA_HEIGHT;
        m_target.fov = CTiltedCameraController::DEFAULT_FOV_RAD;
        m_target.offsetY = CTiltedCameraController::DEFAULT_CAMERA_OFFSET_Y;
        m_target.pitch = CTiltedCameraController::DEFAULT_CAMERA_PITCH * (CTiltedRadarMath::PI / 180.0f);
    }
    else
    {
        if (isPlane)
        {
            m_target.height = CTiltedCameraController::CAMERA_HEIGHT_PLANE;
            m_target.fov = CTiltedCameraController::SPEED_MAX_FOV_RAD;
            m_target.offsetY = CTiltedCameraController::CAMERA_OFFSET_Y_PLANE;
            m_target.pitch = CTiltedCameraController::CAMERA_PITCH_PLANE * (CTiltedRadarMath::PI / 180.0f);
        }
        else if (isHeli)
        {
            m_target.height = CTiltedCameraController::CAMERA_HEIGHT_HELICOPTER;
            m_target.fov = CTiltedCameraController::DEFAULT_FOV_RAD;
            m_target.offsetY = CTiltedCameraController::CAMERA_OFFSET_Y_HELICOPTER;
            m_target.pitch = CTiltedCameraController::CAMERA_PITCH_HELICOPTER * (CTiltedRadarMath::PI / 180.0f);
        }
        else
        {
            if (speedKMH <= 20.0f)
            {
                m_target.height = CTiltedCameraController::VEHICLE_MIN_HEIGHT;
                m_target.fov = CTiltedCameraController::DEFAULT_FOV_RAD;
                m_target.offsetY = CTiltedCameraController::VEHICLE_MIN_OFFSET_Y;
                m_target.pitch = CTiltedCameraController::VEHICLE_MIN_PITCH * (CTiltedRadarMath::PI / 180.0f);
            }
            else if (speedKMH >= 100.0f)
            {
                m_target.height = CTiltedCameraController::SPEED_MAX_HEIGHT;
                m_target.fov = CTiltedCameraController::SPEED_MAX_FOV_RAD;
                m_target.offsetY = CTiltedCameraController::SPEED_MAX_OFFSET_Y;
                m_target.pitch = CTiltedCameraController::SPEED_MAX_PITCH * (CTiltedRadarMath::PI / 180.0f);
            }
            else
            {
                float t = (speedKMH - 20.0f) / 80.0f;
                m_target.height = CTiltedCameraController::VEHICLE_MIN_HEIGHT + (CTiltedCameraController::SPEED_MAX_HEIGHT - CTiltedCameraController::VEHICLE_MIN_HEIGHT) * t;
                m_target.fov = CTiltedCameraController::DEFAULT_FOV_RAD + CTiltedCameraController::FOV_DELTA_RAD * t;
                m_target.offsetY = CTiltedCameraController::VEHICLE_MIN_OFFSET_Y + (CTiltedCameraController::SPEED_MAX_OFFSET_Y - CTiltedCameraController::VEHICLE_MIN_OFFSET_Y) * t;
                float pitchDeg = CTiltedCameraController::VEHICLE_MIN_PITCH + (CTiltedCameraController::SPEED_MAX_PITCH - CTiltedCameraController::VEHICLE_MIN_PITCH) * t;
                m_target.pitch = pitchDeg * (CTiltedRadarMath::PI / 180.0f);
            }
        }
    }

    CTiltedCameraController::UpdateInterpolation();
}

void CTiltedCameraController::UpdateInterpolation()
{
    const float interpolationSpeed = 0.12f;
    m_state.height += (m_target.height - m_state.height) * interpolationSpeed;
    m_state.fov += (m_target.fov - m_state.fov) * interpolationSpeed;
    m_state.offsetX += (m_target.offsetX - m_state.offsetX) * interpolationSpeed;
    m_state.offsetY += (m_target.offsetY - m_state.offsetY) * interpolationSpeed;
    m_state.pitch += (m_target.pitch - m_state.pitch) * interpolationSpeed;
}

void CTiltedCameraController::GetCachedCalculations(float& offsetWorldX, float& offsetWorldY, CTiltedVector3& cameraPos, CTiltedVector3& cameraRot, float cameraHeight)
{
    float invertedYaw = -m_state.yaw;
    if (!m_cache.isValid || fabsf(m_cache.cachedInvertedYaw - invertedYaw) > 0.001f)
    {
        m_cache.cachedInvertedYaw = invertedYaw;
        m_cache.cachedSinYaw = sinf(invertedYaw);
        m_cache.cachedCosYaw = cosf(invertedYaw);
    }

    const float lookAhead = m_state.offsetY;
    const float lateral = m_state.offsetX;
    offsetWorldX = lookAhead * m_cache.cachedSinYaw + lateral * m_cache.cachedCosYaw;
    offsetWorldY = lookAhead * m_cache.cachedCosYaw - lateral * m_cache.cachedSinYaw;

    if (cameraHeight <= 0.0f)
    {
        cameraHeight = m_state.height;
    }

    cameraPos = CTiltedVector3(m_state.posX + offsetWorldX, m_state.posY - offsetWorldY, cameraHeight);
    cameraRot = CTiltedVector3(m_state.pitch, 0.0f, invertedYaw);

    m_cache.cachedCameraPos = cameraPos;
    m_cache.cachedCameraRot = cameraRot;
    m_cache.isValid = true;
}

void CTiltedCameraController::SetState(const CameraState& state)
{
    m_state = state;
}