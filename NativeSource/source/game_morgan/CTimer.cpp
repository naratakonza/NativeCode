#include "../../Main.h"
#include "../../Launcher/MGFlutter.h"
#include "CTimer.h"

#include <mach-o/dyld.h>

bool CTimer::IsUserPause()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x17FC80), "\x08\x2E\x00\xD0") && CMemory::HasMappedInstruction(SA_ADDR(0x17FC88), "\x09\x31\x29\x39") && CMemory::HasMappedInstruction(SA_ADDR(0x17FC94), "\x1F\x31\x29\x39");
    return verified && *reinterpret_cast<const std::uint8_t*>(SA_ADDR(0x741A4C) + _dyld_get_image_vmaddr_slide(0)) != 0;
}

bool CTimer::UpdatePauseState()
{
    const bool paused = CTimer::IsUserPause() || CGame::IsPauseMenuActive() || CGame::IsDrawingBigMap();
    MGSetFlutterPauseState(paused);
    return paused;
}
