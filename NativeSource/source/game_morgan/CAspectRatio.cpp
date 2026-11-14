#include "../../Main.h"
#include "CAspectRatio.h"

#include <mach-o/dyld.h>

static bool cameraSizeHookInstalled = false;

float CAspectRatio::ms_fAspectRatioSize = 0.0f;

void CAspectRatio::Initialise()
{
    CNecessary::Log(OBFUSCATE("CAspectRatio Initialise"));

    cameraSizeHookInstalled = RwCameraSetViewWindow && CMemory::HasMappedInstruction(SA_ADDR(0x315498), "\x80\x08\x00\xB4") && CMemory::HasMappedInstruction(SA_ADDR(0x1593C4), "\xAC\xF7\x0C\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x15A220), "\x15\xF4\x0C\x94");
    if (!cameraSizeHookInstalled)
    {
        CNecessary::Log(OBFUSCATE("CAspectRatio CameraSize bridge unavailable"));
    }
}

void CAspectRatio::CameraSize(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    const auto CameraSize = reinterpret_cast<void (*)(RwCamera*, RwRect*, float, float)>(SA_ADDR(0x315498) + _dyld_get_image_vmaddr_slide(0));
    const float size = CAspectRatio::GetSize();

    CameraSize(camera, rect, viewWindow, aspectRatio);

    if (cameraSizeHookInstalled && camera && size > 0.0f && viewWindow > 0.0f)
    {
        // Keep GTA's raster and viewport sizing. Change only the projection.
        const RwV2d viewWindowSize{viewWindow, viewWindow / size};
        RwCameraSetViewWindow(camera, viewWindowSize);
    }
}

extern "C" __attribute__((visibility("default"))) void CameraSize_hook(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    CAspectRatio::CameraSize(camera, rect, viewWindow, aspectRatio);
}

void CAspectRatio::SetSize(eAspectRatioType type)
{
    switch (type) {
        case eAspectRatioType::ASPECTRATIO_DEFAULT: {
            CAspectRatio::ms_fAspectRatioSize = 0.0f;

            break;
        }

        case eAspectRatioType::ASPECTRATIO_3_2: {
            CAspectRatio::ms_fAspectRatioSize = 3.0f / 2.0f;

            break;
        }

        case eAspectRatioType::ASPECTRATIO_4_3: {
            CAspectRatio::ms_fAspectRatioSize = 4.0f / 3.0f;

            break;
        }

        case eAspectRatioType::ASPECTRATIO_5_4: {
            CAspectRatio::ms_fAspectRatioSize = 5.0f / 4.0f;

            break;
        }

        case eAspectRatioType::ASPECTRATIO_16_9: {
            CAspectRatio::ms_fAspectRatioSize = 16.0f / 9.0f;

            break;
        }

        case eAspectRatioType::ASPECTRATIO_16_10: {
            CAspectRatio::ms_fAspectRatioSize = 16.0f / 10.0f;

            break;
        }

        case eAspectRatioType::ASPECTRATIO_21_9: {
            CAspectRatio::ms_fAspectRatioSize = 21.0f / 9.0f;

            break;
        }

        case eAspectRatioType::ASPECTRATIO_32_9: {
            CAspectRatio::ms_fAspectRatioSize = 32.0f / 9.0f;

            break;
        }
    }
}
