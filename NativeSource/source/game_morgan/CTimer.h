#pragma once

#include <cstdint>

class CTimer
{
public:
    static float& ms_fTimeStep;
    static bool& m_UserPause;
    static std::uint32_t& m_snTimeInMilliseconds;
    static std::uint32_t& m_snTimeInMillisecondsNonClipped;
    static float& game_FPS;

public:
    static void Initialise();
    static void StartUserPause();
    static void EndUserPause();
    static void SetGamePause(std::uint8_t state);
    static void SetGameTimer(std::uint32_t timer);
    static bool IsUserPause();
};
