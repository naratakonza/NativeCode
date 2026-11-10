#include "../../Main.h"
#include "CClock.h"

#include <mach-o/dyld.h>

bool CClock::m_bClockEnabled = true;
static bool passingOfTime = true;

extern "C" __attribute__((visibility("default"))) void MGClockUpdate(void (*original)())
{
    if (passingOfTime)
    {
        original();
    }
}

extern "C" __attribute__((visibility("default"))) int MGClockFormatHUD(char* buffer, int hours, int minutes)
{
    if (!CClock::m_bClockEnabled)
    {
        buffer[0] = '\0';
        return 0;
    }

    return std::snprintf(buffer, sizeof("00:00"), OBFUSCATE("%02d:%02d"), hours, minutes);
}

void CClock::ToggleThePassingOfTime(bool bOnOff)
{
    passingOfTime = bOnOff;
}

void CClock::EnableGameClock(bool bEnable)
{
    if (!CMemory::HasMappedInstruction(SA_ADDR(0x151C2C), "\xFA\x14\x0D\x94") || !CMemory::HasMappedInstruction(SA_ADDR(0x208D28), "\xC0\x38\x0A\x94")) {
        CNecessary::Log(OBFUSCATE("CClock EnableGameClock bridge unavailable"));
        return;
    }

    CClock::ToggleThePassingOfTime(bEnable);
    CClock::m_bClockEnabled = bEnable;
}

void CClock::SetGameTime(int iHour, int iMinute)
{
    if (!pthread_main_np() || !CScripting::gst) {
        return;
    }

    if (CGame::GetActiveInterior() != 0)
    {
        iHour = 14;
        iMinute = 0;
    }

    if (iHour < 0 || iHour >= 24 || iMinute < 0 || iMinute >= 60) {
        return;
    }

    CScripting::ScriptCommand(&set_current_time, iHour, iMinute);
}

void CClock::GetWorldTime(int* iHour, int* iMinute)
{
    if (!iHour || !iMinute || !CMemory::HasMappedInstruction(SA_ADDR(0x13BE74), "\x8C\x2F\x00\xD0") || !CMemory::HasMappedInstruction(SA_ADDR(0x13BE78), "\x8D\x2D\x7C\x39") || !CMemory::HasMappedInstruction(SA_ADDR(0x13BEA4), "\x88\x2F\x00\xD0") || !CMemory::HasMappedInstruction(SA_ADDR(0x13BEA8), "\x09\x29\x7C\x39")) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    *iHour = *reinterpret_cast<const std::uint8_t*>(SA_ADDR(0x72DF0A) + slide);
    *iMinute = *reinterpret_cast<const std::uint8_t*>(SA_ADDR(0x72DF0B) + slide);
}