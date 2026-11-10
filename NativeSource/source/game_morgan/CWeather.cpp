#include "../../Main.h"
#include "CGame.h"
#include "CClock.h"
#include "CWeather.h"

#include <mach-o/dyld.h>

int16_t& CWeather::ForcedWeatherType = *reinterpret_cast<int16_t*>(SA_ADDR(0x8E8EF4) + _dyld_get_image_vmaddr_slide(0));
int16_t& CWeather::OldWeatherType = *reinterpret_cast<int16_t*>(SA_ADDR(0x8E8EF0) + _dyld_get_image_vmaddr_slide(0));
int16_t& CWeather::NewWeatherType = *reinterpret_cast<int16_t*>(SA_ADDR(0x8E8EF2) + _dyld_get_image_vmaddr_slide(0));

static const bool weatherFieldsVerified = CMemory::HasMappedInstruction(SA_ADDR(0x312B78), "\xA8\x2E\x00\xD0") && CMemory::HasMappedInstruction(SA_ADDR(0x312B7C), "\x00\xE9\x1D\x79") && CMemory::HasMappedInstruction(SA_ADDR(0x312B84), "\x00\xE1\x1D\x79") && CMemory::HasMappedInstruction(SA_ADDR(0x312B8C), "\x00\xE5\x1D\x79");

void CWeather::SetGameWeather(eWeatherType byteWeatherID)
{
	if (CGame::GetActiveInterior() != 0)
	{
		byteWeatherID = eWeatherType::WEATHER_SUNNY_DESERT;
	}

	CWeather::ForceWeather(byteWeatherID);

	if (!CClock::m_bClockEnabled)
	{
		CWeather::ForceWeatherOld(byteWeatherID);
	}
}

void CWeather::ForceWeather(eWeatherType weatherType)
{
    if (!weatherFieldsVerified) {
        return;
    }

    CWeather::ForcedWeatherType = weatherType;
}

void CWeather::ForceWeatherNow(eWeatherType weatherType)
{
    if (!weatherFieldsVerified) {
        return;
    }

    CWeather::ForcedWeatherType = weatherType;
    CWeather::OldWeatherType = weatherType;
    CWeather::NewWeatherType = weatherType;
}

void CWeather::ForceWeatherOld(eWeatherType weatherType)
{
    if (!weatherFieldsVerified) {
        return;
    }

    CWeather::NewWeatherType = weatherType;
    CWeather::OldWeatherType = weatherType;
}