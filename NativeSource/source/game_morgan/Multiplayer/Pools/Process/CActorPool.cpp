#include "../../../../../Main.h"
#include "CActorPool.h"
#include "../../../CUtil.h"
#include "../../../CCommon.h"
#include "../../../CStreaming.h"
#include "../../../Enums/eMaxItem.h"
#include "../../Guis/CGui.h"

CActorPool::CActorPool()
{
    CNecessary::Log(OBFUSCATE("CActorPool Initialise"));
}

CActorPool::~CActorPool()
{
    CNecessary::Log(OBFUSCATE("CActorPool Uninitialise"));
    CActorPool::DeleteAll();
}

void CActorPool::DeleteAll()
{
    for (auto actorIter = CActorPool::m_pActors.begin(); actorIter != CActorPool::m_pActors.end();)
    {
        SAFE_DELETE(actorIter->second);
        actorIter = CActorPool::m_pActors.erase(actorIter);
    }

    CActorPool::m_bActorsSlot.clear();
}

bool CActorPool::Add(uint16_t actorId, int iSkin, VECTOR vecPos, float fRotation, float fHealth, bool bInvulnerable)
{
    if (actorId >= eMaxItem::MAX_ACTORS) {
        return false;
    }

    CActorPool::Delete(actorId);

    if (!CCommon::IsPedModel(iSkin)) {
        iSkin = 0;
    }

    if (!CStreaming::IsObjectInCdImage(iSkin, OBFUSCATE("0x09A"))) {
        CNecessary::Log(OBFUSCATE("Actor: ModelID %d is not present in game files (CdImage)"), iSkin);
        iSkin = 0;
    }

    CActor* newActor = new (std::nothrow) CActor(iSkin, vecPos, fRotation);
    if (!newActor) {
        return false;
    }

    newActor->SetHealth(fHealth);
    newActor->SetInvulnerable(bInvulnerable);
    CActorPool::m_pActors.emplace(actorId, newActor);
    CActorPool::m_bActorsSlot[actorId] = true;

    return true;
}

bool CActorPool::Delete(uint16_t actorId, bool eJect)
{
    if (actorId >= eMaxItem::MAX_ACTORS) {
        return false;
    }

    auto actorIter = CActorPool::m_pActors.find(actorId);
    if (actorIter != CActorPool::m_pActors.end())
    {
        if (!eJect)
        {
            if (CGui::GetChat())
            {
                CGui::GetChat()->AddDebugMessage(1, OBFUSCATE("Warning: actor %u was not deleted"), actorId);
            }
        }

        SAFE_DELETE(actorIter->second);
        CActorPool::m_pActors.erase(actorIter);
        CActorPool::m_bActorsSlot[actorId] = false;

        return true;
    }

    return false;
}

uint16_t CActorPool::FindTargetEntityID(CEntityGta* pTargetEntity)
{
    if (!pTargetEntity) {
        return eInvalidId::INVALID_PLAYER_ID;
    }

    for (const auto& actorPair : CActorPool::m_pActors)
    {
        uint16_t actorId = actorPair.first;
        CActor* pActor = actorPair.second;

        if (pActor && pActor->m_pEntity == pTargetEntity)
        {
            return actorId;
        }
    }

    return eInvalidId::INVALID_PLAYER_ID;
}

uint16_t CActorPool::FindIDFromCameraRay(float maxDistance, CVector* cameraPos, CVector* rayEnd, [[maybe_unused]] float radius)
{
    if (!cameraPos || !rayEnd) {
        return eInvalidId::INVALID_ACTOR_ID;
    }

    float closestOffset = 100000.0f;
    uint16_t closestActorId = eInvalidId::INVALID_ACTOR_ID;

    for (const auto& actorPair : CActorPool::m_pActors)
    {
        uint16_t actorId = actorPair.first;
        CActor* pActor = actorPair.second;

        if (pActor && CUtil::GamePool_Ped_GetAt(pActor->m_dwGTAId) && pActor->IsAdded() && pActor->GetDistanceFromCamera() < maxDistance)
		{
            CVector boundCenter;
            pActor->GetBoundCentre(&boundCenter);

            if (pActor->IsPointInsideEntity(cameraPos, rayEnd, &boundCenter, 1.5f, true))
            {
                float fOffsetFromRay = CUtil::GetDistanceBetweenPointAndLine(&boundCenter, cameraPos, rayEnd);
                if (fOffsetFromRay < closestOffset)
                {
                    closestOffset = fOffsetFromRay;
                    closestActorId = actorId;
                }
            }
        }
    }

    return closestActorId;
}