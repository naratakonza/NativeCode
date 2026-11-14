#include "../../Main.h"
#include "CAspectRatio.h"

#include <mach-o/dyld.h>

static bool cameraSizeHookInstalled = false;

float CAspectRatio::ms_fAspectRatioSize = 0.0f;

void CAspectRatio::Initialise()
{
    CNecessary::Log(OBFUSCATE("CAspectRatio Initialise"));

    cameraSizeHookInstalled = RwCameraSetViewWindow && CMemory::HasMappedInstruction(SA_ADDR(0x315498), "\x80\x08\x00\xB4") && CMemory::HasMappedInstruction(SA_ADDR(0x1593C4), "\xAC\xF7\x0C\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x15A220), "\x15\xF4\x0C\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x2AC1E4), "\xFF\xC3\x00\xD1") && CMemory::HasMappedInstruction(SA_ADDR(0x2AC26C), "\xFF\xC3\x00\xD1") && CMemory::HasMappedInstruction(SA_ADDR(0x315594), "\x00\x05\x00\xFD");
    if (!cameraSizeHookInstalled)
    {
        CNecessary::Log(OBFUSCATE("CAspectRatio CameraSize bridge unavailable"));
    }
}

void CAspectRatio::CameraSize(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    if (!camera || !camera->frameBuffer) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    if (!cameraSizeHookInstalled)
    {
        reinterpret_cast<void (*)(RwCamera*, RwRect*, float, float)>(SA_ADDR(0x315498) + slide)(camera, rect, viewWindow, aspectRatio);
        return;
    }

    RwVideoMode modeInfo{};
    int currentVideoMode = reinterpret_cast<int (*)()>(SA_ADDR(0x2AC26C) + slide)();
    if (!reinterpret_cast<RwVideoMode* (*)(RwVideoMode*, int)>(SA_ADDR(0x2AC1E4) + slide)(&modeInfo, currentVideoMode))
    {
        reinterpret_cast<void (*)(RwCamera*, RwRect*, float, float)>(SA_ADDR(0x315498) + slide)(camera, rect, viewWindow, aspectRatio);
        return;
    }

    RwRaster* frameBuffer = camera->frameBuffer;
    int screenWidth = frameBuffer->width;
    int screenHeight = frameBuffer->height;

    RwRect defaultRect;
    if (rect == nullptr)
    {
        defaultRect.x = 0;
        defaultRect.y = 0;
        defaultRect.w = (modeInfo.flags & rwVIDEOMODEEXCLUSIVE) ? screenWidth : modeInfo.width;
        defaultRect.h = (modeInfo.flags & rwVIDEOMODEEXCLUSIVE) ? screenHeight : modeInfo.height;
        rect = &defaultRect;
    }

    if (rect->w <= 0 || rect->h <= 0) {
        return;
    }

    if (screenWidth != rect->w || screenHeight != rect->h)
    {
        if (camera->zBuffer)
        {
            camera->zBuffer->width = rect->w;
            camera->zBuffer->height = rect->h;
        }

        frameBuffer->width = rect->w;
        frameBuffer->height = rect->h;
    }

    RwV2d viewWindowSize;
    if (modeInfo.flags & rwVIDEOMODEEXCLUSIVE)
    {
        viewWindowSize.x = viewWindow;
        if (CAspectRatio::GetSize() == 0.0f)
        {
            viewWindowSize.y = aspectRatio > 0.0f ? viewWindow / aspectRatio : (viewWindow * rect->h) / rect->w;
        }
        else
        {
            viewWindowSize.y = viewWindow / CAspectRatio::GetSize();
        }
    }
    else
    {
        if (rect->w <= rect->h)
        {
            viewWindowSize.x = (viewWindow * rect->w) / rect->h;
            viewWindowSize.y = viewWindow;
        }
        else
        {
            viewWindowSize.x = viewWindow;
            viewWindowSize.y = (viewWindow * rect->h) / rect->w;
        }
    }

    RwCameraSetViewWindow(camera, viewWindowSize);

    auto* rsGlobal = reinterpret_cast<RsGlobalType*>(SA_ADDR(0x918A68) + slide);
    rsGlobal->maximumWidth = rect->w;
    rsGlobal->maximumHeight = rect->h;
}

extern "C" __attribute__((visibility("default"))) void CameraSize_hook(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio)
{
    CAspectRatio::CameraSize(camera, rect, viewWindow, aspectRatio);
}

void CAspectRatio::SetSize(eAspectRatioType type)
{
    switch (type) {
        default:
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
