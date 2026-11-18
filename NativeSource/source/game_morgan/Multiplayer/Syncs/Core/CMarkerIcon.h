#pragma once

#include <cstdint>

class CMarkerIcon
{
public:
    static uint32_t Create(uint8_t iMarkerType, float fX, float fY, float fZ, uint32_t iColor, uint8_t iStyle);
    static void Destroy(uint32_t dwMarkerId);
};