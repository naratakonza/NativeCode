#pragma once

class CClock
{
public:
    static bool m_bClockEnabled;

public:
    static void EnableGameClock(bool bEnable);
    static void ToggleThePassingOfTime(bool bOnOff);
    static void SetGameTime(int iHour, int iMinute);
    static void GetWorldTime(int* iHour, int* iMinute);
};