#pragma once

#include <cstdint>
#include <cstddef>

struct COccluder
{
    int16_t iCenterX;
    int16_t iCenterY;
    int16_t iCenterZ;
    uint16_t iLength;
    uint16_t iWidth;
    uint16_t iHeight;
    uint8_t iRotZ;
    uint8_t iRotY;
    uint8_t iRotX;
    uint8_t m_cPad;
    uint16_t m_nFlags;
};
static_assert(sizeof(COccluder) == 0x12);
static_assert(offsetof(COccluder, iCenterX) == 0x00);
static_assert(offsetof(COccluder, iLength) == 0x06);