#include "../../../../../Main.h"
#include "CGangZonePool.h"
#include "../CNetGamePool.h"
#include "../../Guis/CGui.h"

CGangZonePool::CGangZonePool()
{
    CNecessary::Log(OBFUSCATE("CGangZonePool Initialise"));
    CGangZonePool::m_bInMenuState = false;
}

CGangZonePool::~CGangZonePool()
{
    CNecessary::Log(OBFUSCATE("CGangZonePool Uninitialise"));
    CGangZonePool::DeleteAll();
}

void CGangZonePool::DeleteAll()
{
    for (auto gangZoneIter = CGangZonePool::m_pGangZones.begin(); gangZoneIter != CGangZonePool::m_pGangZones.end();)
    {
        SAFE_DELETE(gangZoneIter->second);
        gangZoneIter = CGangZonePool::m_pGangZones.erase(gangZoneIter);
    }

    CGangZonePool::m_bGangZonesSlot.clear();
    CGangZonePool::m_pSortedGangZones.clear();
}

void CGangZonePool::Draw()
{
    static uint32_t dwLastTick = 0;
    uint32_t dwTick = CNecessary::GetTickCount();
    static uint8_t alt = 0;

    if (dwTick - dwLastTick >= 500)
    {
        alt = ~alt;
        dwLastTick = dwTick;
    }

    for (CGangZone* pGangZone : CGangZonePool::m_pSortedGangZones)
    {
        pGangZone->Draw(CGangZonePool::IsInMenuState(), alt);
    }
}

bool CGangZonePool::Add(uint16_t gangZoneId, float fMinX, float fMinY, float fMaxX, float fMaxY, uint32_t dwColor)
{
    if (gangZoneId >= eMaxItem::MAX_GANG_ZONES) {
        return false;
    }

    CGangZonePool::Delete(gangZoneId);

    CGangZone* newGangZone = new (std::nothrow) CGangZone(fMinX, fMaxY, fMaxX, fMinY, dwColor, dwColor);
    if (!newGangZone) {
        return false;
    }

    CGangZonePool::m_pGangZones.emplace(gangZoneId, newGangZone);
    CGangZonePool::m_bGangZonesSlot[gangZoneId] = true;

    CGangZonePool::m_pSortedGangZones.push_back(newGangZone);
    std::sort(CGangZonePool::m_pSortedGangZones.begin(), CGangZonePool::m_pSortedGangZones.end(), [](const CGangZone* a, const CGangZone* b) {
        return a->GetArea() > b->GetArea();
    });

    return true;
}

bool CGangZonePool::Delete(uint16_t gangZoneId)
{
    if (gangZoneId >= eMaxItem::MAX_GANG_ZONES) {
        return false;
    }

    auto gangZoneIter = CGangZonePool::m_pGangZones.find(gangZoneId);
    if (gangZoneIter != CGangZonePool::m_pGangZones.end())
    {
        CGangZone* pGangZone = gangZoneIter->second;
        auto sortIter = std::find(CGangZonePool::m_pSortedGangZones.begin(), CGangZonePool::m_pSortedGangZones.end(), pGangZone);
        if (sortIter != CGangZonePool::m_pSortedGangZones.end())
        {
            CGangZonePool::m_pSortedGangZones.erase(sortIter);
        }

        SAFE_DELETE(pGangZone);
        CGangZonePool::m_pGangZones.erase(gangZoneIter);
        CGangZonePool::m_bGangZonesSlot[gangZoneId] = false;

        return true;
    }

    return false;
}

void CGangZonePool::SetInMenuState(bool state)
{
    CGangZonePool::m_bInMenuState = state;
}

bool CGangZonePool::IsInMenuState()
{
    return CGangZonePool::m_bInMenuState;
}