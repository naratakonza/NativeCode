#include "../../Main.h"
#include "../../Launcher/MGFlutter.h"
#include "CTimer.h"
#include "Multiplayer/CMultiPlayer.h"

#include <mach-o/dyld.h>

float& CTimer::ms_fTimeStep = *reinterpret_cast<float*>(SA_ADDR(0x741A38) + _dyld_get_image_vmaddr_slide(0));
bool& CTimer::m_UserPause = *reinterpret_cast<bool*>(SA_ADDR(0x741A4C) + _dyld_get_image_vmaddr_slide(0));
std::uint32_t& CTimer::m_snTimeInMilliseconds = *reinterpret_cast<std::uint32_t*>(SA_ADDR(0x741A10) + _dyld_get_image_vmaddr_slide(0));
std::uint32_t& CTimer::m_snTimeInMillisecondsNonClipped = *reinterpret_cast<std::uint32_t*>(SA_ADDR(0x741A14) + _dyld_get_image_vmaddr_slide(0));
float& CTimer::game_FPS = *reinterpret_cast<float*>(SA_ADDR(0x741A44) + _dyld_get_image_vmaddr_slide(0));

static bool timerReady = false;

void CTimer::Initialise()
{
    CNecessary::Log(OBFUSCATE("CTimer Initialise"));

    timerReady = CMemory::HasMappedInstruction(SA_ADDR(0x17FC80), "\x08\x2E\x00\xD0") && CMemory::HasMappedInstruction(SA_ADDR(0x17FC88), "\x09\x31\x29\x39") && CMemory::HasMappedInstruction(SA_ADDR(0x17FC94), "\x1F\x31\x29\x39") && CMemory::HasMappedInstruction(SA_ADDR(0x17FAB4), "\xA9\x12\x0A\xB9");
}

void CTimer::StartUserPause()
{
    CTimer::SetGamePause(true);

    if (timerReady && CMultiPlayer::IsGameActive())
    {
        MGSetFlutterPauseState(true);
    }
}

void CTimer::EndUserPause()
{
    CTimer::SetGamePause(false);

    if (timerReady && CMultiPlayer::IsGameActive())
    {
        MGSetFlutterPauseState(false);
    }
}

void CTimer::SetGamePause(std::uint8_t state)
{
    if (timerReady)
    {
        CTimer::m_UserPause = static_cast<bool>(state);
    }
}

void CTimer::SetGameTimer(std::uint32_t timer)
{
    if (timerReady && !CClock::m_bClockEnabled)
    {
        CTimer::m_snTimeInMilliseconds = timer & 0x3FFFFFFF;
    }
}

bool CTimer::IsUserPause()
{
    return timerReady && CTimer::m_UserPause;
}

extern "C" __attribute__((visibility("default"))) void MGTimerStartUserPause()
{
    CTimer::StartUserPause();
}

extern "C" __attribute__((visibility("default"))) void MGTimerEndUserPause()
{
    CTimer::EndUserPause();
}