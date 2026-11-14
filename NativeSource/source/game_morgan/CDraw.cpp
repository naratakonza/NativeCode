#include "../../Main.h"
#include "CDraw.h"
#include "CCameraAim.h"
#include "CCameraController.h"
#include "CTimer.h"
#include "Enums/eMaxConfig.h"
#include "Multiplayer/CMultiPlayer.h"

#include <mach-o/dyld.h>

float& CDraw::ms_fFOV = *reinterpret_cast<float*>(SA_ADDR(0x4E8AF8) + _dyld_get_image_vmaddr_slide(0));
const float& CDraw::ms_fAspectRatio = *reinterpret_cast<const float*>(SA_ADDR(0x80EB74) + _dyld_get_image_vmaddr_slide(0));
float CDraw::ms_SavefFOV = 77.0f;

static bool setFovHookInstalled = false;

void CDraw::Initialise()
{
    CNecessary::Log(OBFUSCATE("CDraw Initialise"));

    setFovHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x2D7C70), "\xA8\x29\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x2D7C74), "\x01\x75\x4B\xBD") && CMemory::HasMappedInstruction(SA_ADDR(0x2D7CA4), "\x88\x10\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x2D7CA8), "\x00\xF9\x0A\xBD") && CMemory::HasMappedInstruction(SA_ADDR(0x13727C), "\x01\x80\x0D\x94");
    if (!setFovHookInstalled)
    {
        CNecessary::Log(OBFUSCATE("CDraw SetFOV bridge unavailable"));
    }
}

extern "C" __attribute__((visibility("default"))) void CDraw_SetFov_hook(float value, bool isCinematic)
{
    CDraw::SetFovHook(value, isCinematic);
}

void CDraw::SetFovHook(float value, bool isCinematic)
{
    if (setFovHookInstalled && wCameraMode2 && CMultiPlayer::IsSampActive())
    {
        float drawFov = std::min(value, static_cast<float>(eMaxConfig::MAX_FOV));
        if (!CTimer::IsUserPause() && CDraw::GetFov() != 77.0f && !CCameraController::IsCameraAllAiming(false, true))
        {
            drawFov = CDraw::GetFov();
        }

        const float fov = (((CDraw::ms_fAspectRatio - 1.3333f) * 11.0f) / 0.44444f) + drawFov;
        if (fov > 0.0f && fov < 180.0f)
        {
            CDraw::ms_fFOV = fov;
            return;
        }
    }

    reinterpret_cast<void (*)(float, bool)>(SA_ADDR(0x2D7C70) + _dyld_get_image_vmaddr_slide(0))(value, isCinematic);
}

void CDraw::SetFov(float value)
{
    if (!std::isfinite(value) || value < 77.0f) {
        value = 77.0f;
    }

    CDraw::ms_SavefFOV = std::min(value, static_cast<float>(eMaxConfig::MAX_FOV));
}

float CDraw::GetFov()
{
    return CDraw::ms_SavefFOV;
}
