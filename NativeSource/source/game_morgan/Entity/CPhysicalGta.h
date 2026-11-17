#pragma once

#include "CEntityGta.h"
#include "../Core/CVector.h"
#include "../RenderWare/rtquat.h"

class CRealTimeShadow;

struct CPhysicalGta : CEntityGta
{
    float m_fPrevDistFromCam; // 0x5C
    uint32_t m_LastCollisionTime; // 0x60
    union
    {
        struct
        {
            unsigned int bMakeMassTwiceAsBig : 1;
            unsigned int bApplyGravity : 1;
            unsigned int bDisableCollisionForce : 1;
            unsigned int bCollidable : 1;
            unsigned int bDisableTurnForce : 1;
            unsigned int bDisableMoveForce : 1;
            unsigned int bInfiniteMass : 1;
            unsigned int bDisableZ : 1;

            unsigned int bSubmergedInWater : 1;
            unsigned int bOnSolidSurface : 1;
            unsigned int bBroken : 1;
            unsigned int bProcessCollisionEvenIfStationary : 1;
            unsigned int b13 : 1; // only used for peds
            unsigned int bDontApplySpeed : 1;
            unsigned int b15 : 1;
            unsigned int bProcessingShift : 1;

            unsigned int b17 : 1;
            unsigned int bDisableSimpleCollision : 1; // ref @ CPhysical::ProcessCollision
            unsigned int bBulletProof : 1;
            unsigned int bFireProof : 1;
            unsigned int bCollisionProof : 1;
            unsigned int bMeeleProof : 1;
            unsigned int bInvulnerable : 1;
            unsigned int bExplosionProof : 1;

            unsigned int bDontCollideWithFlyers : 1;
            unsigned int bAttachedToEntity : 1;
            unsigned int bAddMovingCollisionSpeed : 1;
            unsigned int bTouchingWater : 1;
            unsigned int bCanBeCollidedWith : 1;
            unsigned int bDestroyed : 1;
            unsigned int b31 : 1;
            unsigned int b32 : 1;
        } physicalFlags;
        unsigned int m_nPhysicalFlags;
    }; // 0x64
    CVector vecMoveSpeed; // 0x68
    CVector m_vecTurnSpeed;
    CVector m_vecMoveFriction;
    CVector m_vecTurnFriction;
    CVector m_vecAverageMoveSpeed;
    CVector m_vecAverageTurnSpeed;
    float m_fMass;
    float m_fTurnMass;
    float m_fVelocityFrequency;
    float m_fAirResistance;
    float m_fElasticity;
    float m_fBuoyancyConstant;
    CVector m_vecCOM;
    uint8_t _pad2[4];
    uintptr_t m_listEntryInfo; // มันควร 8 byte ใน 64bit
    uintptr_t dwUnkModelRel; // เพราะอันนี้ต้องเริ่มต้นที่ 0xE0
    uint8_t m_nNoOfStaticFrames;
    uint8_t m_nNoOfCollisionRecords;
    uint8_t m_LastMaterialToHaveBeenStandingOn;
    uint8_t _pad4[5];
    CEntityGta *m_apCollidedEntities[6];
    float m_fTrueDistanceTravelled;
    float m_fDamageImpulseMagnitude;
    CEntityGta *m_pDamageEntity;
    CVector m_vecLastCollisionImpactVelocity;
    CVector m_vecLastCollisionPosn;
    uint16_t m_nPieceType;
    uint8_t pad01[6];
    CEntityGta *m_pAttachToEntity;
    CVector m_vecAttachPosnOffset;
    CVector m_vecAttachTiltOffset;
    RtQuat m_AttachQuat;
    CEntityGta *m_pNOCollisionVehicle;
    float m_lightingFromCollision;
    float m_lightingFromPointLights;
    CRealTimeShadow* m_pRealTimeShadow;
};
static_assert(sizeof(CPhysicalGta) == 0x198);

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(CPhysicalGta, m_fPrevDistFromCam) == 0x5C);
static_assert(offsetof(CPhysicalGta, m_LastCollisionTime) == 0x60);
static_assert(offsetof(CPhysicalGta, m_nPhysicalFlags) == 0x64);
static_assert(offsetof(CPhysicalGta, vecMoveSpeed) == 0x68);
static_assert(offsetof(CPhysicalGta, m_listEntryInfo) == 0xD8);
static_assert(offsetof(CPhysicalGta, m_pAttachToEntity) == 0x150);
static_assert(offsetof(CPhysicalGta, m_AttachQuat) == 0x170);
static_assert(offsetof(CPhysicalGta, m_pRealTimeShadow) == 0x190);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif