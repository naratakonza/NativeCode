#include "../../../../../Main.h"
#include "CMapIconPool.h"
#include "../../Syncs/Core/CMarkerIcon.h"

uint32_t CMapIconPool::m_dwMapIcons[eMaxItem::MAX_MAP_ICONS] = { 0 };

void CMapIconPool::Add(uint8_t byteIndex, float fX, float fY, float fZ, uint8_t byteIcon, uint32_t iColor, uint8_t style)
{
    if (byteIndex >= ARRAY_SIZE(CMapIconPool::m_dwMapIcons))
    {
        return;
    }

    CMapIconPool::Delete(byteIndex);
    CMapIconPool::m_dwMapIcons[byteIndex] = static_cast<uint32_t>(CMarkerIcon::Create(byteIcon, fX, fY, fZ, iColor, style));
}

void CMapIconPool::Delete(uint8_t byteIndex)
{
    if (byteIndex >= ARRAY_SIZE(CMapIconPool::m_dwMapIcons))
    {
        return;
    }

    if (CMapIconPool::m_dwMapIcons[byteIndex])
    {
        CMarkerIcon::Destroy(m_dwMapIcons[byteIndex]);
        CMapIconPool::m_dwMapIcons[byteIndex] = 0;
    }
}

void CMapIconPool::DeleteAll()
{
    for (std::size_t MapIconId = 0; MapIconId < ARRAY_SIZE(CMapIconPool::m_dwMapIcons); MapIconId++)
    {
        CMapIconPool::Delete(static_cast<uint8_t>(MapIconId));
    }
}