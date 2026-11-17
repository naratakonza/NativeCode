#pragma once

#include "../../../Enums/eInvalidId.h"
#include "../../../Entity/Actor/CActor.h"
#include <unordered_map>

class CActorPool
{
private:
    std::unordered_map<uint16_t, CActor*> m_pActors;
    std::unordered_map<uint16_t, bool> m_bActorsSlot;

public:
    CActorPool();
    ~CActorPool();

    void DeleteAll();

    bool Add(uint16_t actorId, int iSkin, VECTOR vecPos, float fRotation, float fHealth, bool bInvulnerable);
    bool Delete(uint16_t actorId, bool eJect = false);

    CActor* GetActor(uint16_t ID) {
        auto it = CActorPool::m_pActors.find(ID);
        if (it != CActorPool::m_pActors.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool GetActorInSlot(uint16_t ID) {
        auto it = CActorPool::m_bActorsSlot.find(ID);
        if (it != CActorPool::m_bActorsSlot.end()) {
            return it->second;
        }

        return false;
    }

    uint16_t FindTargetEntityID(CEntityGta* pTargetEntity);
    uint16_t FindIDFromCameraRay(float maxDistance, CVector* cameraPos, CVector* rayEnd, float radius);
};