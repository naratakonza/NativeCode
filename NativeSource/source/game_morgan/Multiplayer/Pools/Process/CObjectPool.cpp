#include "../../../../../Main.h"
#include "CObjectPool.h"
#include "../../Guis/CGui.h"
#include "../../../CGame.h"
#include "../../../CCommon.h"
#include "../../../CStreaming.h"
#include "../../CMultiPlayer.h"
#include "../../../Base/Optimize/CGtaObjectCache.h"

CObjectPool::CObjectPool()
{
    CNecessary::Log(OBFUSCATE("CObjectPool Initialise"));

    // if (CGui::GetChat())
    // {
    //     CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("CObjectPool Initialise"));
    // }
}

CObjectPool::~CObjectPool()
{
    CNecessary::Log(OBFUSCATE("CObjectPool Uninitialise"));
    CObjectPool::DeleteAll();
}

void CObjectPool::Process()
{
    static uint32_t s_ulongLastCall = CNecessary::GetTickCount();

    uint32_t ulongTick = CNecessary::GetTickCount();
    float fElapsedTime = static_cast<float>(ulongTick - s_ulongLastCall) / 1000.0f;

    for (const auto& objPair : CObjectPool::m_pObjects)
    {
        CObject* pObject = objPair.second;

        if (pObject)
        {
            pObject->Process(fElapsedTime);
        }
    }

    s_ulongLastCall = ulongTick;
}

void CObjectPool::Draw()
{
    for (const auto& objPair : CObjectPool::m_pObjects)
    {
        CObject* pObject = objPair.second;

        if (pObject)
        {
            pObject->Draw();
        }
    }
}

void CObjectPool::ProcessMaterialText()
{
    for (const auto& objPair : CObjectPool::m_pObjects)
    {
        CObject* pObject = objPair.second;

        if (pObject)
        {
            pObject->CreateMaterialFontTexture();
        }
    }
}

void CObjectPool::DeleteAll()
{
    for (auto objIter = CObjectPool::m_pObjects.begin(); objIter != CObjectPool::m_pObjects.end();)
    {
        CObjectPool::EjectCameraFromObject(objIter->first);

        SAFE_DELETE(objIter->second);
        objIter = CObjectPool::m_pObjects.erase(objIter);
    }

    CObjectPool::m_bObjectsSlot.clear();
}

bool CObjectPool::Add(uint16_t ObjectID, int modelID, CVector vecPos, CVector vecRot, float fDrawDistance)
{
    if (ObjectID >= eMaxItem::MAX_OBJECTS) {
        return false;
    }

    CObjectPool::Delete(ObjectID);

    if (!CCommon::IsExistForModelInfo(modelID)) {
        modelID = eSpecialObject::OBJECT_NOMODELFILE;
    }

    if (!CStreaming::IsObjectInCdImage(modelID, OBFUSCATE("0x107"))) {
        CNecessary::Log(OBFUSCATE("Object: ModelID %d is not present in game files (CdImage)"), modelID);
        return false;
    }

    if (!CCommon::ModelInfoGetCol(modelID, OBFUSCATE("0x107"))) {
        CNecessary::Log(OBFUSCATE("Object: Collision data not found for ModelID %d"), modelID);
        return false;
    }

    CObject* newObject = new (std::nothrow) CObject(modelID, vecPos.x, vecPos.y, vecPos.z, vecRot, fDrawDistance);
    if (!newObject) {
        return false;
    }

    CObjectPool::m_pObjects.emplace(ObjectID, newObject); // หากระบบไหน มีการเรียก Delete ให้ใช้ emplace แทน เพราะจะไม่เกิด overwrite (ปลอดภัยกว่า)
    CObjectPool::m_bObjectsSlot[ObjectID] = true;
    CGtaObjectCache::SetDirty();

    return true;
}

void CObjectPool::Delete(uint16_t ObjectID)
{
    // CNecessary::Log(OBFUSCATE("CObjectPool::Delete %d"), ObjectID);

    if (ObjectID >= eMaxItem::MAX_OBJECTS) {
        return;
    }

    auto objIter = CObjectPool::m_pObjects.find(ObjectID);
    if (objIter != CObjectPool::m_pObjects.end())
    {
        CObjectPool::EjectCameraFromObject(ObjectID);

        SAFE_DELETE(objIter->second);
        CObjectPool::m_pObjects.erase(objIter);
        CObjectPool::m_bObjectsSlot[ObjectID] = false;
        CGtaObjectCache::SetDirty();
    }
}

void CObjectPool::EjectCameraFromObject(uint16_t ObjectID)
{
    CObject* pObject = CObjectPool::GetObject(ObjectID);
    if (!pObject) {
        return;
    }

    // CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
    // if (pPlayerPool)
    // {
    //     if (pPlayerPool->GetLocalPlayer() && pPlayerPool->GetLocalPlayer()->m_surfData.pObject == pObject)
    //     {
    //         pPlayerPool->GetLocalPlayer()->EndSurfing();
    //     }
    // }
    
    if (CGame::GetCamera() && CGame::GetCamera()->GetAttach() == pObject)
    {
        CGame::GetCamera()->Attach(nullptr);
    }
}

CObject* CObjectPool::GetObjectFromGtaPtr(CEntityGta* pGtaObject) const
{
    if (!pGtaObject) {
        return nullptr;
    }

    for (const auto& objPair : CObjectPool::m_pObjects)
    {
        CObject* pObject = objPair.second;

        if (pObject && pObject->m_pEntity == pGtaObject)
        {
            return pObject;
        }
    }

    return nullptr;
}

uint16_t CObjectPool::FindIDFromGtaPtr(CEntityGta* pGtaObject) const
{
    if (!pGtaObject) {
        return eInvalidId::INVALID_OBJECT_ID;
    }

    for (const auto& objPair : CObjectPool::m_pObjects)
    {
        [[maybe_unused]] uint16_t objectId = objPair.first;
        CObject* pObject = objPair.second;

        if (pObject && pObject->m_pEntity == pGtaObject)
        {
            return objectId;
        }
    }

    return eInvalidId::INVALID_OBJECT_ID;
}

uint16_t CObjectPool::FindIDFromCameraRay(float maxDistance, CVector* cameraPos, CVector* rayEnd)
{
    uint16_t closestObjectId = eInvalidId::INVALID_OBJECT_ID;
    bool bIsPointInsideEntity = false;

    for (const auto& objPair : CObjectPool::m_pObjects)
    {
        uint16_t objectId = objPair.first;
        CObject* pObject = objPair.second;

        if (pObject && pObject->IsRenderable())
        {
            float fDistanceToTargetPoint = pObject->GetDistanceFromPoint(*cameraPos);
            uint16_t modelIndex = pObject->GetModelIndex();

            if (modelIndex != 19300 && fDistanceToTargetPoint < maxDistance)
            {
                uintptr_t colModel = CCommon::GetColModelByID(modelIndex);
                if (colModel)
                {
                    uintptr_t colData = *reinterpret_cast<uintptr_t*>(colModel + 0x30);
                    if (pObject->GetCollisionChecking() && colData)
                    {
                        bIsPointInsideEntity = pObject->TestSphereCastVsEntity(reinterpret_cast<uintptr_t>(cameraPos), reinterpret_cast<uintptr_t>(rayEnd), false);
                    }
                    else
                    {
                        CVector boundCenter;
                        pObject->GetBoundCentre(&boundCenter);

                        bIsPointInsideEntity = pObject->IsPointInsideEntity(cameraPos, rayEnd, &boundCenter, 1.0f, true);
                    }

                    if (bIsPointInsideEntity)
                    {
                        maxDistance = fDistanceToTargetPoint;
                        closestObjectId = objectId;
                    }
                }
            }
        }
    }

    return closestObjectId;
}

bool CObjectPool::ObjectCollisionProcess(uintptr_t CColSphere1, uintptr_t CColSphere2)
{
    for (const auto& objPair : CObjectPool::m_pObjects)
    {
        [[maybe_unused]] uint16_t objectId = objPair.first;
        CObject* pObject = objPair.second;

        if (pObject->IsRenderable() && !pObject->m_bDontCollideWithCamera && pObject->GetDistanceFromCamera() < 200.0f && !pObject->IsEntityIgnored() && pObject->TestSphereCastVsEntity(CColSphere1, CColSphere2, true))
        {
            return true;
        }
    }

    return false;
}

bool CObjectPool::IsValidPointer(uint16_t ObjectID, CObject* pObject, bool m_bDebug)
{
    auto objIter = CObjectPool::m_pObjects.find(ObjectID);
    if (objIter == CObjectPool::m_pObjects.end() || objIter->second != pObject)
    {
        if (m_bDebug)
        {
            // CException::Print(0, 0, 0, OBFUSCATE("ObjectProcess"));
        }

        return false;
    }

    return true;
}