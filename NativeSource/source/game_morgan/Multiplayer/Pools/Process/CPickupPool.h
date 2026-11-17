#pragma once

#include "../../../CPickup.h"
#include "../../../Enums/eSpecialObject.h"
#include "../../../Enums/eMaxItem.h"

class CPickupPool
{
private:
    std::unordered_map<uint16_t, CPickup*> m_pPickups;
    std::unordered_map<uint16_t, int> m_iTimer;

public:
    CPickupPool();
    ~CPickupPool();
    void Process();
    void DeleteAll();

    bool Add(int ModelID, int SpawnType, float fX, float fY, float fZ, int PickupID);
    bool Delete(int PickupID);

    CPickup* GetPickup(uint16_t id) {
        auto it = CPickupPool::m_pPickups.find(id);
        if (it != CPickupPool::m_pPickups.end()) {
            return it->second;
        }

        return nullptr;
    }

    int GetTimer(uint16_t id) {
        auto it = CPickupPool::m_iTimer.find(id);
        if (it != CPickupPool::m_iTimer.end()) {
            return it->second;
        }

        return 0;
    }

    void SendPickedUpPickup(int id);
    void SendPickedUpPickupByGTAId(int PickupID);
    int GetNumberFromID(int PickupID);
};