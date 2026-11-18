#pragma once

#include <unordered_map>
#include <vector>

#include "../../../CGangZone.h"
#include "../../../Enums/eMaxItem.h"

class CGangZonePool
{
private:
    std::unordered_map<uint16_t, CGangZone*> m_pGangZones;
    std::unordered_map<uint16_t, bool> m_bGangZonesSlot;
    std::vector<CGangZone*> m_pSortedGangZones;
    bool m_bInMenuState;

public:
    CGangZonePool();
    ~CGangZonePool();
    void Draw();
    void Process() {}
    void DeleteAll();

    bool Add(uint16_t gangZoneId, float fMinX, float fMinY, float fMaxX, float fMaxY, uint32_t dwColor);
    bool Delete(uint16_t gangZoneId);

    void SetInMenuState(bool state);
    bool IsInMenuState();

    CGangZone* GetGangZone(uint16_t id) {
        auto it = CGangZonePool::m_pGangZones.find(id);
        if (it != CGangZonePool::m_pGangZones.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool GetGangZoneInSlot(uint16_t id) {
        auto it = CGangZonePool::m_bGangZonesSlot.find(id);
        if (it != CGangZonePool::m_bGangZonesSlot.end()) {
            return it->second;
        }

        return false;
    }
};