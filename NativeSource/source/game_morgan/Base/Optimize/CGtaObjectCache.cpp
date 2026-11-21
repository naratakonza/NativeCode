#include "../../../../Main.h"
#include "CGtaObjectCache.h"
#include "../../Multiplayer/CMultiPlayer.h"
#include "../../Multiplayer/Pools/CNetGamePool.h"

std::unordered_map<CEntityGta*, CObject*> CGtaObjectCache::m_ObjectCache;
std::unordered_map<CEntityGta*, CObject*> CGtaObjectCache::m_AccessoryCache;
bool CGtaObjectCache::m_bNeedsUpdate = true;
uint32_t CGtaObjectCache::m_dwLastUpdateTick = 0;

void CGtaObjectCache::Update()
{
    uint32_t currentTick = CNecessary::GetTickCount();
    if (!CGtaObjectCache::m_bNeedsUpdate && (currentTick - CGtaObjectCache::m_dwLastUpdateTick) < 250) {
        return;
    }

    CGtaObjectCache::m_dwLastUpdateTick = currentTick;
    CGtaObjectCache::m_bNeedsUpdate = false;

    CGtaObjectCache::m_ObjectCache.clear();
    CGtaObjectCache::m_AccessoryCache.clear();

    if (!CMultiPlayer::IsSampActive()) {
        return;
    }

    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()) {
        return;
    }

    // CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
    // if (pPlayerPool)
    // {
    //     CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();
    //     if (pLocalPlayer)
    //     {
    //         CPlayerPed* pPlayerPed = pLocalPlayer->GetPlayerPed();
    //         if (pPlayerPed)
    //         {
    //             for (int i = 0; i < eMaxItem::MAX_ATTACHED_OBJECTS; ++i)
    //             {
    //                 auto& accessory = pPlayerPed->GetAccessory(i);
    //                 if (accessory.pObject && accessory.pObject->m_pEntity)
    //                 {
    //                     CGtaObjectCache::m_AccessoryCache[accessory.pObject->m_pEntity] = accessory.pObject;
    //                 }
    //             }
    //         }
    //     }

    //     for (const auto& playerPair : pPlayerPool->GetAllRemotePlayers())
    //     {
    //         CRemotePlayer* pPlayer = playerPair.second;
    //         if (pPlayer)
    //         {
    //             CPlayerPed* pGamePlayer = pPlayer->GetGamePlayer();
    //             if (pGamePlayer)
    //             {
    //                 for (int i = 0; i < eMaxItem::MAX_ATTACHED_OBJECTS; ++i)
    //                 {
    //                     auto& accessory = pGamePlayer->GetAccessory(i);
    //                     if (accessory.pObject && accessory.pObject->m_pEntity)
    //                     {
    //                         CGtaObjectCache::m_AccessoryCache[accessory.pObject->m_pEntity] = accessory.pObject;
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }
    CObjectPool* pObjectPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetObjectPool();
    if (pObjectPool)
    {
        for (int i = 0; i < eMaxItem::MAX_OBJECTS; ++i)
        {
            CObject* pObject = pObjectPool->GetObject(i);
            if (pObject && pObject->m_pEntity)
            {
                CGtaObjectCache::m_ObjectCache[pObject->m_pEntity] = pObject;
            }
        }
    }
}

void CGtaObjectCache::Remove(CObject* pObject)
{
    if (!pObject) {
        return;
    }

    for (auto objectIter = CGtaObjectCache::m_ObjectCache.begin(); objectIter != CGtaObjectCache::m_ObjectCache.end(); )
    {
        if (objectIter->second == pObject)
        {
            objectIter = CGtaObjectCache::m_ObjectCache.erase(objectIter);
        }
        else
        {
            ++objectIter;
        }
    }

    for (auto objectIter = CGtaObjectCache::m_AccessoryCache.begin(); objectIter != CGtaObjectCache::m_AccessoryCache.end(); )
    {
        if (objectIter->second == pObject)
        {
            objectIter = CGtaObjectCache::m_AccessoryCache.erase(objectIter);
        }
        else
        {
            ++objectIter;
        }
    }
}

CObject* CGtaObjectCache::FindObject(CEntityGta* pEntity)
{
    auto objectIter = CGtaObjectCache::m_ObjectCache.find(pEntity);
    if (objectIter != CGtaObjectCache::m_ObjectCache.end())
    {
        return objectIter->second;
    }

    return nullptr;
}

CObject* CGtaObjectCache::FindAccessory(CEntityGta* pEntity)
{
    auto accessoryIter = CGtaObjectCache::m_AccessoryCache.find(pEntity);
    if (accessoryIter != CGtaObjectCache::m_AccessoryCache.end())
    {
        return accessoryIter->second;
    }

    return nullptr;
}
