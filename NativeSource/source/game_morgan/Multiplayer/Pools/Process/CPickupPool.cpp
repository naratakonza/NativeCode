#include "../../../../../Main.h"
#include "CPickupPool.h"
#include "../../../CCommon.h"
#include "../../../CStreaming.h"
#include "../../Guis/CGui.h"
#include "../../CMultiPlayer.h"

CPickupPool::CPickupPool() 
{
    CNecessary::Log(OBFUSCATE("CPickupPool Initialise"));
}

CPickupPool::~CPickupPool()
{
    CNecessary::Log(OBFUSCATE("CPickupPool Uninitialise"));
    CPickupPool::DeleteAll();
}

void CPickupPool::Process() 
{
    for (const auto& pickupPair : CPickupPool::m_pPickups)
    {
        uint16_t pickupId = pickupPair.first;
        CPickup* pPickup = pickupPair.second;

        if (!pPickup || pPickup->GetHandle() == 0) {
            continue;
        }

        if (pPickup->GetType() == 14) 
        {
            if (pPickup->IsPickedUp()) 
            {
                RakNet::BitStream bsPickup;
                bsPickup.Write(pickupId);
                CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_PickedUpPickup, &bsPickup, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, 0);
            }
        }

        if (CPickupPool::m_iTimer[pickupId] > 0) 
        {
            CPickupPool::m_iTimer[pickupId]--;
        }
    }
}

void CPickupPool::DeleteAll()
{
    for (auto pickupIter = CPickupPool::m_pPickups.begin(); pickupIter != CPickupPool::m_pPickups.end();)
    {
        SAFE_DELETE(pickupIter->second);
        pickupIter = CPickupPool::m_pPickups.erase(pickupIter);
    }

    CPickupPool::m_iTimer.clear();
}

bool CPickupPool::Add(int ModelID, int SpawnType, float fX, float fY, float fZ, int PickupID)
{
    if (PickupID < 0 || PickupID >= eMaxItem::MAX_PICKUPS) {
        return false;
    }

    CPickupPool::Delete(PickupID);

    if (!CCommon::IsExistForModelInfo(ModelID)) {
        ModelID = eSpecialObject::OBJECT_NOMODELFILE;
    }

    if (!CStreaming::IsObjectInCdImage(ModelID, OBFUSCATE("0x213"))) {
        CNecessary::Log(OBFUSCATE("Pickup: ModelID %d is not present in game files (CdImage)"), ModelID);
        return false;
    }

    if (!CCommon::ModelInfoGetCol(ModelID, OBFUSCATE("0x213"))) {
        CNecessary::Log(OBFUSCATE("Pickup: Collision data not found for ModelID %d"), ModelID);
        return false;
    }

    int pickupId = 0;
    CPickup* newPickup = new (std::nothrow) CPickup(ModelID, SpawnType, fX, fY, fZ, &pickupId);
    if (!newPickup) {
        return false;
    }

    newPickup->SetID(pickupId);
    CPickupPool::m_pPickups.emplace(PickupID, newPickup);
    CPickupPool::m_iTimer[PickupID] = 0;

    return true;
}

bool CPickupPool::Delete(int PickupID)
{
    if (PickupID < 0 || PickupID >= eMaxItem::MAX_PICKUPS) {
        return false;
    }

    auto pickupIter = CPickupPool::m_pPickups.find(PickupID);
    if (pickupIter != CPickupPool::m_pPickups.end())
    {
        if (pickupIter->second && pickupIter->second->GetHandle() != 0)
		{
            SAFE_DELETE(pickupIter->second);
        }

        CPickupPool::m_pPickups.erase(pickupIter);
        CPickupPool::m_iTimer[PickupID] = 0;

        return true;
    }

    return false;
}

void CPickupPool::SendPickedUpPickup(int id)
{
    RakNet::BitStream bsPickup;
    bsPickup.Write(id);
    CMultiPlayer::GetActiveNetwork()->GetRakClient()->RPC(&RPC_PickedUpPickup, &bsPickup, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, 0);
}

void CPickupPool::SendPickedUpPickupByGTAId(int PickupID)
{
    if (PickupID < 0 || PickupID >= eMaxItem::MAX_PICKUPS) {
        return;
    }

    int index = CPickupPool::GetNumberFromID(PickupID);

    if (index >= 0 && CPickupPool::GetTimer(index) <= 0)
    {
        CPickupPool::SendPickedUpPickup(index);
        CPickupPool::m_iTimer[index] = 15;
    }
}

int CPickupPool::GetNumberFromID(int PickupID)
{
    for (const auto& pickupPair : CPickupPool::m_pPickups)
    {
        CPickup* pPickup = pickupPair.second;
        if (pPickup && pPickup->GetID() == PickupID) {
            return pickupPair.first;
        }
    }

    return -1;
}