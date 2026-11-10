#pragma once

#include "Enums/eWeatherType.h"

class CWeather
{
public:
    static int16_t& ForcedWeatherType;
    static int16_t& OldWeatherType;
    static int16_t& NewWeatherType;

public:
    static void SetGameWeather(eWeatherType byteWeatherID);
    static void ForceWeather(eWeatherType weatherType);
    static void ForceWeatherNow(eWeatherType weatherType);
    static void ForceWeatherOld(eWeatherType weatherType);

};