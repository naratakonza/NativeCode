#pragma once

#include "../../../Enums/eMaxItem.h"

class CMapIconPool
{
private:
    static uint32_t m_dwMapIcons[eMaxItem::MAX_MAP_ICONS];

public:
    static void Add(uint8_t byteIndex, float fX, float fY, float fZ, uint8_t byteIcon, uint32_t iColor, uint8_t style);
    static void Delete(uint8_t byteIndex);
    static void DeleteAll();
};