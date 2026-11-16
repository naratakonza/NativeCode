#include "../../../../Main.h"
#include "CAudio.h"
#include "../../Enums/eSoundLoadedState.h"

#include <mach-o/dyld.h>

// CAudioStream* CAudio::pAudioStream = nullptr;
bool CAudio::m_bOutdoorAmbienceTrackDisabled = false;
std::uint8_t CAudio::m_bSoundLoaded = eSoundLoadedState::SOUND_NOT_LOADED;

void CAudio::Initialise()
{
    CNecessary::Log(OBFUSCATE("CAudio Initialise"));

    // CAudio::pAudioStream = new CAudioStream();
}

void CAudio::Uninitialise()
{
    CNecessary::Log(OBFUSCATE("CAudio Uninitialise"));

    // SAFE_DELETE(CAudio::pAudioStream);
}

void CAudio::Process()
{
    if (CAudio::m_bSoundLoaded != eSoundLoadedState::SOUND_LOADING) {
        return;
    }

    if (CScripting::ScriptCommand(&get_mission_audio, 1))
    {
        CScripting::ScriptCommand(&play_mission_audio, 1);
        CAudio::m_bSoundLoaded = eSoundLoadedState::SOUND_LOADED;
    }
}

void CAudio::Play(int iSound, float fX, float fY, float fZ)
{
    if (iSound)
    {
        if (iSound == 1)
        {
            CAudio::m_bOutdoorAmbienceTrackDisabled = true;
        }
        else if (iSound >= 1000)
        {
            if (iSound >= 2000)
            {
                CScripting::ScriptCommand(&clear_mission_audio, 1);
                CScripting::ScriptCommand(&load_mission_audio, 1, iSound);
                CScripting::ScriptCommand(&set_mission_audio_position, 1, fX, fY, fZ);
                CAudio::m_bSoundLoaded = eSoundLoadedState::SOUND_LOADING;
            }
            else
            {
                CScripting::ScriptCommand(&play_sound, fX, fY, fZ, iSound);
            }
        }
        else
        {
            CAudio::SetOutdoorAmbienceTrack(iSound);
        }
    }
    else
    {
        if (CAudio::m_bSoundLoaded)
        {
            CScripting::ScriptCommand(&clear_mission_audio, 1);
            CAudio::m_bSoundLoaded = eSoundLoadedState::SOUND_NOT_LOADED;
        }

        CAudio::StopOutdoorAmbienceTrack();
        CAudio::m_bOutdoorAmbienceTrackDisabled = false;
    }
}

bool CAudio::IsOutdoorAmbienceTrackDisabled()
{
    return CAudio::m_bOutdoorAmbienceTrackDisabled;
}

void CAudio::SetOutdoorAmbienceTrack(int iSound)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x048ACC), "\x01\x0C\x00\xB9\xC0\x03\x5F\xD6", 8, "__text") && CMemory::HasMappedInstruction(SA_ADDR(0x06311C), "\x6C\x96\xFF\x17");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    reinterpret_cast<void (*)(uintptr_t, int)>(SA_ADDR(0x048ACC) + slide)(SA_ADDR(0x4C277C) + slide, iSound);
}

void CAudio::StopOutdoorAmbienceTrack()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x048AD4), "\x08\x08\x40\xB9\x1F\x0D\x00\x71", 8, "__text") && CMemory::HasMappedInstruction(SA_ADDR(0x0619FC), "\x36\x9C\xFF\x17");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x048AD4) + slide)(SA_ADDR(0x4C277C) + slide);
}