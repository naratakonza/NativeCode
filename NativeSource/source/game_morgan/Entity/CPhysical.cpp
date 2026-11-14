#include "../../../Main.h"
#include "CPhysical.h"
#include "CEntity.h"
#include "../CUtil.h"
#include "../CTimer.h"

#include <mach-o/dyld.h>

float CPhysical::m_fGravity = -0.008f;

void CPhysical::Initialise()
{
    CNecessary::Log(OBFUSCATE("CPhysical Initialise"));

    const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x162788), "\x78\xD3\x0C\x14") && CMemory::HasMappedBytes(SA_ADDR(0x497568), "\xD0\x33\x00\x90\x10\xE6\x73\x39\x90\x00\x00\x34\xF0\x00\x00\xD0\x10\x62\x42\xF9\x00\x02\x1F\xD6\xE9\x23\xBD\x6D\x82\x2C\xF3\x17", 32, "__mg_gameproc");
    if (!verified) {
        return;
    }

    *reinterpret_cast<std::uint8_t*>(SA_ADDR(0xB0FCF9) + _dyld_get_image_vmaddr_slide(0)) = 1;
}

void CPhysical::ProcessControl(CPhysicalGta *thiz)
{
    CEntityGta* m_pDamageEntity;

    if ((thiz->nControlFlags & 7) != 3)
    {
        thiz->m_nPhysicalFlags &= ~0x100;
    }

    thiz->m_nEntityFlags &= 0xFFFFEF97;

    if ((thiz->nControlFlags & 0xF8) != 16)
    {
        m_pDamageEntity = thiz->m_pDamageEntity;
        thiz->m_nNoOfCollisionRecords = 0;
        thiz->m_nPieceType = 0;
        thiz->m_nPhysicalFlags = thiz->m_nPhysicalFlags & 0xBFFFFDFF;
        thiz->m_fDamageImpulseMagnitude = 0.0f;

        if (m_pDamageEntity)
        {
            CEntity::CleanUpOldReference(m_pDamageEntity, &thiz->m_pDamageEntity);
        }

        thiz->m_pDamageEntity = nullptr;
        CPhysical::ApplyFriction(thiz);

        if (!thiz->m_pAttachToEntity || (thiz->m_nPhysicalFlags & 0x40) != 0)
        {
            CPhysical::ApplyGravity(thiz);
            CPhysical::ApplyAirResistance(thiz);
        }
    }
}

void CPhysical::ApplyGravity(CPhysicalGta *thiz)
{
    CVector vecForce;
    CVector vecOffset;

    if ((thiz->m_nPhysicalFlags & 0x22) == 2)
    {
        if ((thiz->m_nPhysicalFlags & 0x40) != 0)
        {
            vecOffset = CUtil::Multiply3x3(thiz->mat, &thiz->m_vecCOM);
            vecForce.x = 0.0;
            vecForce.y = 0.0;
            vecForce.z = (float)(CTimer::ms_fTimeStep * CPhysical::GetGravity()) * thiz->m_fMass;
            CPhysical::ApplyForce(thiz, vecForce, vecOffset, true);
        }
        else if ((thiz->m_nEntityFlags & 1) != 0)
        {
            thiz->vecMoveSpeed.z = thiz->vecMoveSpeed.z + (float)(CTimer::ms_fTimeStep * CPhysical::GetGravity());
        }
    }
}

void CPhysical::SetGravity(float fGravity)
{
    CPhysical::m_fGravity = -fGravity;
}

float CPhysical::GetGravity()
{
    return CPhysical::m_fGravity;
}

void CPhysical::ApplyForce(CPhysicalGta *thiz, CVector vecForce, CVector vecOffset, bool bValidTorque)
{
    reinterpret_cast<void (*)(CPhysicalGta *, CVector, CVector, bool)>(SA_ADDR(0x160F44) + _dyld_get_image_vmaddr_slide(0))(thiz, vecForce, vecOffset, bValidTorque);
}

void CPhysical::ApplyAirResistance(CPhysicalGta *thiz)
{
    reinterpret_cast<void (*)(CPhysicalGta *)>(SA_ADDR(0x162504) + _dyld_get_image_vmaddr_slide(0))(thiz);
}

void CPhysical::ApplyFriction(CPhysicalGta *thiz)
{
    reinterpret_cast<void (*)(CPhysicalGta *)>(SA_ADDR(0x162324) + _dyld_get_image_vmaddr_slide(0))(thiz);
}