#include "../../Main.h"
#include "CTouchInterface.h"

#include <mach-o/dyld.h>

int CTouchInterface::IsTouched(int WidgetID, void* pVecOut, int nFrameCount)
{
    static const auto IsTouched = CMemory::HasMappedInstruction(SA_ADDR(0x1FB414), "\xF8\x5F\xBC\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1FB4B8), "\x02\x55\x40\xF9")
        ? reinterpret_cast<int (*)(int, void*, int)>(SA_ADDR(0x1FB414) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return IsTouched ? IsTouched(WidgetID, pVecOut, nFrameCount) : 0;
}

int CTouchInterface::IsReleased(int WidgetID, void* pVecOut, int nFrameCount)
{
    static const auto IsReleased = CMemory::HasMappedInstruction(SA_ADDR(0x1FB594), "\xF8\x5F\xBC\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1FB638), "\x02\x39\x40\xF9")
        ? reinterpret_cast<int (*)(int, void*, int)>(SA_ADDR(0x1FB594) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return IsReleased ? IsReleased(WidgetID, pVecOut, nFrameCount) : 0;
}

int CTouchInterface::IsJustPressed(int WidgetID, void* pVecOut, int nFrameCount)
{
    static const auto IsJustPressed = CMemory::HasMappedInstruction(SA_ADDR(0x1FB4D4), "\xF8\x5F\xBC\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1FB578), "\x02\x35\x40\xF9")
        ? reinterpret_cast<int (*)(int, void*, int)>(SA_ADDR(0x1FB4D4) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return IsJustPressed ? IsJustPressed(WidgetID, pVecOut, nFrameCount) : 0;
}

int CTouchInterface::IsSwipedRight(int WidgetID, int nFrameCount)
{
    static const auto IsSwipedRight = CMemory::HasMappedInstruction(SA_ADDR(0x1FB7C0), "\xF6\x57\xBD\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1FB858), "\x01\x4D\x40\xF9")
        ? reinterpret_cast<int (*)(int, int)>(SA_ADDR(0x1FB7C0) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return IsSwipedRight ? IsSwipedRight(WidgetID, nFrameCount) : 0;
}

int CTouchInterface::IsSwipedLeft(int WidgetID, int nFrameCount)
{
    static const auto IsSwipedLeft = CMemory::HasMappedInstruction(SA_ADDR(0x1FB714), "\xF6\x57\xBD\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1FB7AC), "\x01\x49\x40\xF9")
        ? reinterpret_cast<int (*)(int, int)>(SA_ADDR(0x1FB714) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return IsSwipedLeft ? IsSwipedLeft(WidgetID, nFrameCount) : 0;
}

bool CTouchInterface::IsHeldDown(int WidgetID, int nFrameCount)
{
    static const auto IsHeldDown = CMemory::HasMappedInstruction(SA_ADDR(0x1FB86C), "\xF6\x57\xBD\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1FB904), "\x01\x2D\x40\xF9")
        ? reinterpret_cast<bool (*)(int, int)>(SA_ADDR(0x1FB86C) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return IsHeldDown ? IsHeldDown(WidgetID, nFrameCount) : false;
}

bool CTouchInterface::IsPinchZooming(int WidgetID, void* pVecOut, int nFrameCount)
{
    static const auto IsPinchZooming = CMemory::HasMappedInstruction(SA_ADDR(0x1FB654), "\xF8\x5F\xBC\xA9") && CMemory::HasMappedInstruction(SA_ADDR(0x1FB6F8), "\x02\x31\x40\xF9")
        ? reinterpret_cast<bool (*)(int, void*, int)>(SA_ADDR(0x1FB654) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    return IsPinchZooming ? IsPinchZooming(WidgetID, pVecOut, nFrameCount) : false;
}