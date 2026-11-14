#include "../../Main.h"
#include "CCameraController.h"
#include "CCameraAim.h"

#include <mach-o/dyld.h>

bool CCameraController::m_bAimed = false;
float CCameraController::fVerticalAngle;
float CCameraController::fHorizontalAngle;

static bool (*IsLockOnMode)() = nullptr;

void CCameraController::Initialise()
{
    CNecessary::Log(OBFUSCATE("CCameraController Initialise"));

    if (CMemory::HasMappedInstruction(SA_ADDR(0x1EF7F8), "\xFD\x7B\xBF\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1EF800), "\x52\x71\xFF\x97") && CMemory::HasMappedInstruction(SA_ADDR(0x1EF810), "\x08\x39\x40\xB9"))
    {
        IsLockOnMode = reinterpret_cast<bool (*)()>(SA_ADDR(0x1EF7F8) + _dyld_get_image_vmaddr_slide(0));
    }

    CCameraController::m_bAimed = false;
    if (m_fVerticalAngle && m_fHorizontalAngle)
    {
        CCameraController::fVerticalAngle = *m_fVerticalAngle;
        CCameraController::fHorizontalAngle = *m_fHorizontalAngle;
    }
}

bool CCameraController::IsCameraAllAiming(bool reset, bool zoomOnly)
{
    if (!wCameraMode2) {
        return false;
    }

    const auto mode = static_cast<eCameraMode>(*wCameraMode2);
    if (mode == eCameraMode::SNIPER_MODE || mode == eCameraMode::RPG_MODE || mode == eCameraMode::ROCKET_MODE || mode == eCameraMode::CAMERA_MODE)
    {
        return true;
    }

    if (!zoomOnly && mode == eCameraMode::AIMING_STATE)
    {
        if (reset && IsLockOnMode && IsLockOnMode())
        {
            *wCameraMode2 = 0;
        }

        return true;
    }

    return false;
}

void CCameraController::AimForTheCamera()
{
    if (!pbyteCameraMode || !m_fVerticalAngle || !m_fHorizontalAngle) {
        return;
    }

    switch (*pbyteCameraMode) {
        case eCamMode::MODE_SNIPER:
        case eCamMode::MODE_ROCKETLAUNCHER:
        case eCamMode::MODE_CAMERA:
        case eCamMode::MODE_ROCKETLAUNCHER_HS:
        case eCamMode::MODE_AIMWEAPON: {
            if (!CCameraController::m_bAimed)
            {
                CCameraController::SetAimAngles(CCameraController::fVerticalAngle, CCameraController::fHorizontalAngle);
                CCameraController::m_bAimed = true;
            }
            else
            {
                CCameraController::fVerticalAngle = *m_fVerticalAngle;
                CCameraController::fHorizontalAngle = *m_fHorizontalAngle;
            }

            break;
        }
        default: {
            if (CCameraController::m_bAimed)
            {
                CCameraController::SetAimAngles(CCameraController::fVerticalAngle, CCameraController::fHorizontalAngle);
                CCameraController::m_bAimed = false;
            }
            else
            {
                CCameraController::fVerticalAngle = *m_fVerticalAngle;
                CCameraController::fHorizontalAngle = *m_fHorizontalAngle;
            }

            break;
        }
    }
}

void CCameraController::SetAimAngles(float verticalAngle, float horizontalAngle)
{
    if (m_fVerticalAngle && m_fHorizontalAngle)
    {
        *m_fVerticalAngle = verticalAngle;
        *m_fHorizontalAngle = horizontalAngle;
    }
}
