#include "../../Main.h"
#include "CSensitivity.h"
#include "CCamera.h"
#include "Enums/eMaxConfig.h"

#include <mach-o/dyld.h>

float& CSensitivity::ms_fOnfootTouch = *reinterpret_cast<float*>(SA_ADDR(0x4E1FA4) + _dyld_get_image_vmaddr_slide(0));
float CSensitivity::ms_fAimTouch = 170.0f;
float CSensitivity::ms_fIncarTouch = 150.0f;

void CSensitivity::SetOnfootTouch(float value)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x11F13C), "\x08\x1E\x00\xD0\x00\xA5\x4F\xBD", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x1254B4), "\xE8\x1D\x00\x90\x01\xA5\x4F\xBD", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x152DF8), "\xC9\x2E\x00\xF0\x29\xE1\x32\x91\x4A\xE1\x9A\x52\x6A\x64\xA7\x72\x2A\x01\x00\xB9", 20, "__text");
    if (!verified) {
        CNecessary::Log(OBFUSCATE("CSensitivity iOS camera bindings unavailable"));
        return;
    }

    if (!std::isfinite(value)) {
        return;
    }

    value = std::clamp(value, 0.0f, static_cast<float>(eMaxConfig::MAX_SENSITIVITY));
    CSensitivity::ms_fOnfootTouch = 0.001f + value / 3000.0f;
    CCamera::m_fMouseAccelHorzntl = CSensitivity::ms_fOnfootTouch;
}

void CSensitivity::SetAimTouch(float value)
{
    if (std::isfinite(value))
    {
        CSensitivity::ms_fAimTouch = std::clamp(value, 0.0f, static_cast<float>(eMaxConfig::MAX_SENSITIVITY));
    }
}

float CSensitivity::GetAimTouch()
{
    return CSensitivity::ms_fAimTouch;
}

void CSensitivity::SetIncarTouch(float value)
{
    if (std::isfinite(value))
    {
        CSensitivity::ms_fIncarTouch = std::clamp(value, 0.0f, static_cast<float>(eMaxConfig::MAX_SENSITIVITY));
    }
}

float CSensitivity::GetIncarTouch()
{
    return CSensitivity::ms_fIncarTouch;
}
