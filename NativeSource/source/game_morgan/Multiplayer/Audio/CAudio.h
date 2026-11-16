#pragma once

#include "Stream/CAudioStream.h"

class CAudio
{
private:
    static CAudioStream* pAudioStream;
    static bool m_bOutdoorAmbienceTrackDisabled;
    static std::uint8_t m_bSoundLoaded;

public:
    static void Initialise();
    static void Uninitialise();

    static void Process();
    static void Play(int iSound, float fX, float fY, float fZ);
    static bool IsOutdoorAmbienceTrackDisabled();

    static void SetOutdoorAmbienceTrack(int iSound);
    static void StopOutdoorAmbienceTrack();

    static CAudioStream* GetAudioStream() {
        return pAudioStream;
    }
};