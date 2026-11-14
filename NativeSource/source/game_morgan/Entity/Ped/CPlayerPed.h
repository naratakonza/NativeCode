#pragma once

#include "../CEntity.h"
#include "CPedGta.h"

class CPlayerPed : public CEntity
{
public:
    CPlayerPed(); // local
    CPlayerPed(uint8_t bytePlayerNumber, int iSkin, float fX, float fY, float fZ, float fRotation); // remote
    ~CPlayerPed() override;

    bool VerifyInstance();

    void SetModelIndex(unsigned int uiModel);
    void DestroyFollowPedTask();
    bool IsInVehicle();
    void RemoveFromVehicleAndPutAt(float fX, float fY, float fZ);

    void SetHealth(float fHealth);
    void SetArmour(float fArmour);

    float GetHealth();
    float GetArmour();

public:
    CPedGta* GetPed() {
        return m_pPed;
    };

    CPhysicalGta* GetEntity() {
        return m_pEntity;
    }

public:
    CPedGta* m_pPed = nullptr;
    uint8_t m_bytePlayerNumber = 0;

public:
    static bool m_bRebuildPlayer;

    static void ProcessAnimGroups(bool bUseCJWalk);
};