#include "../../Main.h"
#include "CBuildingRemove.h"
#include "CUtil.h"
#include "CCommon.h"
#include "CPools.h"
#include "COcclusion.h"

#include <mach-o/dyld.h>

int CBuildingRemove::RemovedModelIDs[eMaxPool::MAX_REMOVE_BUILDINGS]{};
CVector CBuildingRemove::RemovedPositions[eMaxPool::MAX_REMOVE_BUILDINGS]{};
float CBuildingRemove::RemovedRadius[eMaxPool::MAX_REMOVE_BUILDINGS]{};
int CBuildingRemove::TotalRemovedObjects = 0;

void CBuildingRemove::Add(int modelID, CVector position, float radius)
{
    if (modelID == 19300) {
        return;
    }

    if (CBuildingRemove::TotalRemovedObjects >= eMaxPool::MAX_REMOVE_BUILDINGS) {
        return;
    }

    CBuildingRemove::RemovedModelIDs[CBuildingRemove::TotalRemovedObjects] = modelID;
    CBuildingRemove::RemovedPositions[CBuildingRemove::TotalRemovedObjects].x = position.x;
    CBuildingRemove::RemovedPositions[CBuildingRemove::TotalRemovedObjects].y = position.y;
    CBuildingRemove::RemovedPositions[CBuildingRemove::TotalRemovedObjects].z = position.z;
    CBuildingRemove::RemovedRadius[CBuildingRemove::TotalRemovedObjects] = radius;

    CBuildingRemove::TotalRemovedObjects++;

    CBuildingRemove::Process(modelID, position.x, position.y, position.z, radius);
}

void CBuildingRemove::Delete()
{
    for (int i = 0; i < CBuildingRemove::TotalRemovedObjects; ++i)
    {
        CBuildingRemove::RemovedModelIDs[i] = 0;
        CBuildingRemove::RemovedPositions[i].x = 0.0f;
        CBuildingRemove::RemovedPositions[i].y = 0.0f;
        CBuildingRemove::RemovedPositions[i].z = 0.0f;
        CBuildingRemove::RemovedRadius[i] = 0.0f;
    }

    CBuildingRemove::TotalRemovedObjects = 0;
}

void CBuildingRemove::Process(int modelID, float posX, float posY, float posZ, float radius)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x16CEA4), "\x13\xE1\x07\xF9");
    if (!verified) {
        return;
    }

    CVector vecPos;
    CVector vecRemove;
    vecRemove.x = posX;
    vecRemove.y = posY;
    vecRemove.z = posZ;

    CBuildingRemove::RemoveOcclusionsInRadius(posX, posY, posZ, 500.0f); // ลบการบังของออฟเจคในระยะ

    CPools* pBuildingPool = *reinterpret_cast<CPools**>(SA_ADDR(0x734FE8) + _dyld_get_image_vmaddr_slide(0));
    if (pBuildingPool && pBuildingPool->objects && pBuildingPool->flags)
    {
        uintptr_t pObjects = reinterpret_cast<uintptr_t>(pBuildingPool->objects);
        int nCount = pBuildingPool->count;
        if (nCount > eMaxPool::MAX_BUILDING_POOL)
        {
            nCount = eMaxPool::MAX_BUILDING_POOL;
        }

        for (int i = 0; i < nCount; i++)
        {
            if (pBuildingPool->flags[i] & 0x80) {
                continue;
            }

            CEntityGta* pEntity = reinterpret_cast<CEntityGta*>(pObjects + (i * 0x60));
            if (!pEntity || !pEntity->vtable) {
                continue;
            }

            if (!CCommon::IsGameEntityArePlaceable(pEntity) && (pEntity->nModelIndex == modelID || modelID == -1))
            {
                if (pEntity->mat)
                {
                    vecPos.x = pEntity->mat->pos.x;
                    vecPos.y = pEntity->mat->pos.y;
                    vecPos.z = pEntity->mat->pos.z;
                }
                else
                {
                    vecPos.x = pEntity->m_transform.m_vPosn.x;
                    vecPos.y = pEntity->m_transform.m_vPosn.y;
                    vecPos.z = pEntity->m_transform.m_vPosn.z;
                }

                if (CUtil::GetDistance(vecPos, vecRemove) <= radius)
                {
                    CBuildingRemove::MoveEntityOutOfView(pEntity);
                }
            }
        }
    }

    CPools* pDummyPool = *reinterpret_cast<CPools**>(SA_ADDR(0x734FF8) + _dyld_get_image_vmaddr_slide(0));
    if (pDummyPool && pDummyPool->objects && pDummyPool->flags)
    {
        uintptr_t pObjects = reinterpret_cast<uintptr_t>(pDummyPool->objects);
        int nCount = pDummyPool->count;
        if (nCount > eMaxPool::MAX_DUMMY_POOL)
        {
            nCount = eMaxPool::MAX_DUMMY_POOL;
        }

        for (int i = 0; i < nCount; i++)
        {
            if (pDummyPool->flags[i] & 0x80) {
                continue;
            }

            CEntityGta* pEntity = reinterpret_cast<CEntityGta*>(pObjects + (i * 0x60));
            if (!pEntity || !pEntity->vtable) {
                continue;
            }

            if (!CCommon::IsGameEntityArePlaceable(pEntity) && (pEntity->nModelIndex == modelID || modelID == -1))
            {
                if (pEntity->mat)
                {
                    vecPos.x = pEntity->mat->pos.x;
                    vecPos.y = pEntity->mat->pos.y;
                    vecPos.z = pEntity->mat->pos.z;
                }
                else
                {
                    vecPos.x = pEntity->m_transform.m_vPosn.x;
                    vecPos.y = pEntity->m_transform.m_vPosn.y;
                    vecPos.z = pEntity->m_transform.m_vPosn.z;
                }

                if (CUtil::GetDistance(vecPos, vecRemove) <= radius)
                {
                    CBuildingRemove::MoveEntityOutOfView(pEntity);
                }
            }
        }
    }

    CPools* pObjectPool = *reinterpret_cast<CPools**>(SA_ADDR(0x734FF0) + _dyld_get_image_vmaddr_slide(0));
    if (pObjectPool && pObjectPool->objects && pObjectPool->flags)
    {
        uintptr_t pObjects = reinterpret_cast<uintptr_t>(pObjectPool->objects);
        int nCount = pObjectPool->count;
        if (nCount > eMaxPool::MAX_OBJECT_POOL)
        {
            nCount = eMaxPool::MAX_OBJECT_POOL;
        }

        for (int i = 0; i < nCount; i++)
        {
            if (pObjectPool->flags[i] & 0x80) {
                continue;
            }

            CEntityGta* pEntity = reinterpret_cast<CEntityGta*>(pObjects + (i * 0x220));
            if (!pEntity || !pEntity->vtable) {
                continue;
            }

            if (!CCommon::IsGameEntityArePlaceable(pEntity) && (pEntity->nModelIndex == modelID || modelID == -1))
            {
                if (pEntity->mat)
                {
                    vecPos.x = pEntity->mat->pos.x;
                    vecPos.y = pEntity->mat->pos.y;
                    vecPos.z = pEntity->mat->pos.z;
                }
                else
                {
                    vecPos.x = pEntity->m_transform.m_vPosn.x;
                    vecPos.y = pEntity->m_transform.m_vPosn.y;
                    vecPos.z = pEntity->m_transform.m_vPosn.z;
                }

                if (CUtil::GetDistance(vecPos, vecRemove) <= radius)
                {
                    CBuildingRemove::MoveEntityOutOfView(pEntity);
                }
            }
        }
    }
}

void CBuildingRemove::RemoveOcclusionsInRadius(float posX, float posY, float posZ, float radius)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2F6928), "\x4B\x2B\x00\xB0\x6B\x29\x4D\xB9", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x2F6940), "\x2E\x2B\x00\xB0\xCE\x61\x1B\x91", 8, "__text");
    if (!verified) {
        return;
    }

    uint32_t numOccluders = *reinterpret_cast<uint32_t*>(SA_ADDR(0x85FD28) + _dyld_get_image_vmaddr_slide(0));

    if (numOccluders > 0 && numOccluders <= 1000)
    {
        CVector vecOcc, vecPos;
        vecPos.x = posX;
        vecPos.y = posY;
        vecPos.z = posZ;

        COccluder* pOccluder = reinterpret_cast<COccluder*>(SA_ADDR(0x85B6D8) + _dyld_get_image_vmaddr_slide(0)); // iOS COcclusion::aOccluders; center starts at +0x00

        for (uint32_t i = 0; i < numOccluders; i++)
        {
            vecOcc.x = pOccluder[i].iCenterX * 0.25f;
            vecOcc.y = pOccluder[i].iCenterY * 0.25f;
            vecOcc.z = pOccluder[i].iCenterZ * 0.25f;

            if (CUtil::GetDistance(vecOcc, vecPos) < radius)
            {
                pOccluder[i].iCenterX = 0;
                pOccluder[i].iCenterY = 0;
                pOccluder[i].iCenterZ = 0;
                pOccluder[i].iLength = 0;
                pOccluder[i].iWidth = 0;
                pOccluder[i].iHeight = 0;
            }
        }
    }
}

void CBuildingRemove::MoveEntityOutOfView(CEntityGta* pEntity)
{
    if (pEntity)
    {
        pEntity->m_transform.m_vPosn.z -= 2000.0f;
        if (pEntity->mat)
        {
            pEntity->mat->pos.z -= 2000.0f;
        }

        pEntity->m_areaCode = 1;
    }
}

bool CBuildingRemove::IsNeedRemoveObject(int modelID, float posX, float posY, float posZ)
{
    if (CBuildingRemove::TotalRemovedObjects == 0) {
        return false;
    }

    CVector vecPos;
    vecPos.x = posX;
    vecPos.y = posY;
    vecPos.z = posZ;

    for (int i = 0; i < CBuildingRemove::TotalRemovedObjects; i++)
    {
        if (CBuildingRemove::RemovedModelIDs[i] == -1 || CBuildingRemove::RemovedModelIDs[i] == modelID)
        {
            if (CUtil::GetDistance(vecPos, CBuildingRemove::RemovedPositions[i]) <= CBuildingRemove::RemovedRadius[i])
            {
                return true;
            }
        }
    }

    return false;
}