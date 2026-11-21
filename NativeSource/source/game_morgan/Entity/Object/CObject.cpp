#include "../../../../Main.h"
#include "CObject.h"
#include "../../CGame.h"
#include <mach-o/dyld.h>
#include <unistd.h>
#include "../../CUtil.h"
#include "../../CTimer.h"
#include "../../CWorld.h"
#include "../../CCommon.h"
#include "../../CSprite2d.h"
#include "../../CStreaming.h"
#include "../../CTxdManager.h"
#include "../../Multiplayer/Guis/CGui.h"
#include "../../Multiplayer/CMultiPlayer.h"
#include "../../Scripts/CScripting.h"
#include "../../Base/Optimize/CGtaObjectCache.h"

static bool directModelCreation = false;

CObject::CObject(int iModel, float fPosX, float fPosY, float fPosZ, CVector vecRot, [[maybe_unused]] float fDrawDistance, bool bAccessory)
{
    m_pEntity = nullptr;
    m_dwGTAId = 0;

    CObject::m_iAlpha = 255;
    CObject::m_byteSyncRotation = 1;
    CObject::m_bDontCollideWithCamera = false;

    CObject::m_attachedToVehicle = eInvalidId::INVALID_VEHICLE_ID;
    CObject::m_attachedToObject = eInvalidId::INVALID_OBJECT_ID;
    CObject::m_bCanChangeAccessoryMaterial = bAccessory;
    CObject::m_bAccessoryMaterialChanged = false;
    CObject::m_vecAttachOffset = CVector();
    CObject::m_vecAttachRotation = CVector();

    CObject::m_bDrawLast = (CCommon::GetModelFlags(iModel) & 2) != 0;
    // CBaseModelInfo* modelInfo = reinterpret_cast<CBaseModelInfo*>(CCommon::GetModelInfoByID(iModel));
    // if (fDrawDistance != 0.0f && modelInfo && CCommon::IsObjectModel(modelInfo))
    // {
    //  modelInfo->m_fDrawDistance = 100;
    // }

    if (iModel < 0)
    {
        CObject::SetDirectModelCreation(true);
    }

    uint32_t dwRetID;
    CScripting::ScriptCommand(&create_object, iModel, fPosX, fPosY, fPosZ, &dwRetID);

    if (iModel < 0)
    {
        CObject::SetDirectModelCreation(false);
    }

    CPhysicalGta* pEntity = CUtil::GamePool_Object_GetAt(dwRetID);
    if (dwRetID && pEntity)
    {
        m_pEntity = pEntity;
        m_dwGTAId = dwRetID;

        CObject::m_iModel = iModel;
        CObject::m_byteMoving = 0;
        CObject::m_fMoveSpeed = 0.0f;
        CObject::m_bNeedRotate = false;

        // if (CCommon::IsObjectModel(modelInfo))
        // {
        //  CObject::m_fDrawDistance = modelInfo->m_fDrawDistance;
        // }
        // else
        // {
        //  CObject::m_fDrawDistance = fDrawDistance;
        // }

        CMatrix mat;
        GetMatrix(&mat);
        mat.pos.x = fPosX;
        mat.pos.y = fPosY;
        mat.pos.z = fPosZ;
        SetMatrix(mat);

        CObject::InstantRotate(vecRot.x, vecRot.y, vecRot.z);
        CObject::SetFlags();

        CObject::m_dwStreamingTimer = 0;
        CObject::m_fRadius = CCommon::ModelInfoGetColSphereRadius(CObject::m_iModel);
        CObject::UpdateRenderPosition();
        CGtaObjectCache::SetDirty();
    }

    CObject::m_bLankTexture = nullptr;
    CObject::m_bHasMaterial = false;
    CObject::m_bCanUpdateMaterial = false;
    CObject::m_bMaterialTextureChanged = false;
    CObject::m_bMaterialColorChanged = false;
    CObject::m_pLastMaterialRwObject = nullptr;

    for (int i = 0; i < MAX_MATERIALS_PER_MODEL; i++)
    {
        memset(&m_Material[i], 0, sizeof(MATERIAL_OBJECT));
    }
}

CObject::~CObject()
{
    CGtaObjectCache::SetDirty();
    CGtaObjectCache::Remove(this);

    m_pEntity = CUtil::GamePool_Object_GetAt(m_dwGTAId);
    if (m_pEntity && !CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        if (GetRWObject())
        {
            DeleteRWObject();
        }

        CScripting::ScriptCommand(&destroy_object, m_dwGTAId);

        if (!CCommon::ModelInfoGetReferenceCount(CObject::m_iModel))
        {
            CStreaming::RemoveModel(CObject::m_iModel, false);
        }
    }

    CObject::DestroyMaterials();
}

void CObject::DestroyMaterials()
{
    if (CObject::m_bHasMaterial)
    {
        for (int i = 0; i < MAX_MATERIALS_PER_MODEL; i++)
        {
            auto& material = CObject::m_Material[i];
            if (material.pMaterialTexture && material.pMaterialTexture != CObject::m_bLankTexture)
            {
                CSprite2d::Destructor(material.pMaterialTexture);
                material.pMaterialTexture = nullptr;
            }

            memset(&material, 0, sizeof(MATERIAL_OBJECT));
        }

        CObject::m_bHasMaterial = false;
    }
}

void CObject::Process(float fElapsedTime)
{
    // if (CObject::m_bHasMaterial || CObject::m_fDrawDistance > 300.0 /*|| CObject::m_bAllowChangeObjectMaterial[10]*/)
    // {
    //  CObject::ProcessStreaming();
    // }
    // else if (!GetRWObject() && CGame::GetPlayer())
    // {
    //  if (CGame::GetPlayer()->IsInVehicle())
    //  {
    //      if (CObject::m_fDrawDistance - 30.0f > GetDistanceFromCamera())
    //      {
    //          if (!CStreaming::IsModelLoaded(CObject::m_iModel))
    //          {
    //              CStreaming::RequestModel(CObject::m_iModel);
    //          }
    //      }
    //  }
    // }

    // CVehiclePool* pVehiclePool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool();
    CObjectPool* pObjectPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetObjectPool();

    // if (CObject::m_attachedToVehicle != eInvalidId::INVALID_VEHICLE_ID)
    // {
    //     Vehicle
    //     if (pVehiclePool)
    //     {
    //         CObject::AttachToVehicle(pVehiclePool->GetVehicle(CObject::m_attachedToVehicle));
    //     }
    // }
    if (CObject::m_attachedToObject != eInvalidId::INVALID_OBJECT_ID)
    {
        if (pObjectPool)
        {
            CObject::AttachToObject(pObjectPool->GetObject(CObject::m_attachedToObject));
        }
    }

    m_pEntity = CUtil::GamePool_Object_GetAt(m_dwGTAId);
    if (!m_pEntity || !m_pEntity->mat) {
        return;
    }

    if (CObject::m_byteMoving & 1)
    {
        CVector vecSpeed = {0.0f, 0.0f, 0.0f};
        CMatrix mat;
        GetMatrix(&mat);

        float prevX = mat.pos.x;
        float prevY = mat.pos.y;
        float prevZ = mat.pos.z;

        float distance = fElapsedTime * CObject::m_fMoveSpeed;
        float remaining = CObject::DistanceRemaining(&mat, &m_matTarget);

        uint32_t dwThisTick = CNecessary::GetTickCount();
        float f1 = ((float)(dwThisTick - CObject::m_dwMoveTick)) * 0.001f * CObject::m_fMoveSpeed;
        float f2 = CObject::m_fDistanceToTargetPoint - remaining;

        if (distance >= remaining)
        {
            SetMoveSpeedVector(vecSpeed);
            SetTurnSpeedVector(vecSpeed);
            mat.pos.x = CObject::m_matTarget.pos.x;
            mat.pos.y = CObject::m_matTarget.pos.y;
            mat.pos.z = CObject::m_matTarget.pos.z;

            if (CObject::m_bNeedRotate)
            {
                CObject::m_quatTarget.GetMatrix(&mat);
            }

            UpdateMatrix(mat);
            CObject::StopMoving();
            return;
        }

        if (fElapsedTime <= 0.0f) {
            return;
        }

        float t = distance / remaining;
        if (t > 1.0f)
        {
            t = 1.0f;
        }

        mat.pos.x += (CObject::m_matTarget.pos.x - mat.pos.x) * t;
        mat.pos.y += (CObject::m_matTarget.pos.y - mat.pos.y) * t;
        mat.pos.z += (CObject::m_matTarget.pos.z - mat.pos.z) * t;

        float delta = 1.0f / fElapsedTime;
        vecSpeed.x = (mat.pos.x - prevX) * delta * 0.02f;
        vecSpeed.y = (mat.pos.y - prevY) * delta * 0.02f;
        vecSpeed.z = (mat.pos.z - prevZ) * delta * 0.02f;

        if (fabs(f1 - f2) > 0.1f)
        {
            if (f1 > f2)
            {
                float adjust = (f1 - f2) * 0.1f + 1.0f;
                vecSpeed.x *= adjust;
                vecSpeed.y *= adjust;
                vecSpeed.z *= adjust;
            }
            else if (f2 > f1)
            {
                float adjust = 1.0f - (f2 - f1) * 0.1f;
                vecSpeed.x *= adjust;
                vecSpeed.y *= adjust;
                vecSpeed.z *= adjust;
            }
        }

        SetMoveSpeedVector(vecSpeed);
        CObject::ApplyMoveSpeed();

        if (CObject::m_bNeedRotate)
        {
            float slerpDelta = 1.0f - (remaining / CObject::m_fDistanceToTargetPoint);
            if (slerpDelta < 0.0f)
            {
                slerpDelta = 0.0f;
            }
            if (slerpDelta > 1.0f)
            {
                slerpDelta = 1.0f;
            }

            CQuaternion quat;
            quat.Slerp(&m_quatStart, &m_quatTarget, slerpDelta);
            quat.Normalize();
            quat.GetMatrix(&mat);

            CVector turnSpeed = {0.0f, 0.0f, 0.0f};
            turnSpeed.z = (CObject::m_quatTarget.z - CObject::m_quatStart.z) * slerpDelta / fElapsedTime;
            if (fabs(turnSpeed.z) > 0.001f)
            {
                turnSpeed.z = (turnSpeed.z > 0.0f) ? 0.001f : -0.001f;
            }

            SetTurnSpeedVector(turnSpeed);
        }

        UpdateMatrix(mat);
    }
}

void CObject::ProcessStreaming()
{
    float fDistance = GetDistanceFromCamera();
    CObject::m_fDistanceToCamera = fDistance;

    if (fDistance > CObject::m_fDrawDistance)
    {
        if (GetRWObject())
        {
            CObject::m_dwStreamingTimer++;
            if (CObject::m_dwStreamingTimer == 1000)
            {
                DeleteRWObject();
                CObject::m_dwStreamingTimer = 0;

                if (CObject::m_iModel > 0 && !CCommon::ModelInfoGetReferenceCount(CObject::m_iModel))
                {
                    CStreaming::RemoveModel(CObject::m_iModel, false);
                }
            }
        }
    }
    else
    {
        if (!GetRWObject())
        {
            if (!CStreaming::IsModelLoaded(CObject::m_iModel))
            {
                CStreaming::RequestModel(CObject::m_iModel);
                return;
            }

            SetModelIndex(CObject::m_iModel);
        }

        if (GetRWObject())
        {
            CObject::m_dwStreamingTimer = 0;

            if (m_pEntity)
            {
                if (!CCommon::IsGameEntityArePlaceable(m_pEntity))
                {
                    CCommon::SetModelFlags(CObject::m_iModel);
                }
            }
        }
    }
}

void CObject::Draw()
{
    if (!m_pEntity || !CGame::GetCamera()) {
        return;
    }

    if (CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    uintptr_t pRwObject = GetRWObject();
    if (!pRwObject) {
        return;
    }

    CObject::UpdateRenderPosition();

    if (CGame::GetCamera()->GetDistanceToPoint(&m_vecSpawnPos) >= CObject::m_fRadius && !CCommon::FrustumTestSphere(&m_vecSpawnPos, CObject::m_fRadius)) {
        return;
    }

    if (CObject::m_bDrawLast)
    {
        uint8_t currArea = CGame::GetActiveInterior();
        if (currArea || (CCommon::GetModelFlags(CObject::m_iModel) & 0x08) != 0)
        {
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
        }
        else
        {
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)100);
        }
    }

    if ((CCommon::GetModelFlags(CObject::m_iModel) & 0x40) != 0)
    {
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)2);
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
    }

    if (CObject::m_bHasMaterial && CObject::m_bCanUpdateMaterial)
    {
        CObject::HandleCustomMaterialUpdate();
    }

    if (CObject::m_fDistanceToCamera < 600.0f)
    {
        if (CObject::m_bHasMaterial && CObject::m_bMaterialColorChanged)
        {
            // เหมือนจะส่งผลบัค texture กระพริบ
            CObject::SetupAccessoryLighting();
            CCommon::RenderClumpOrAtomic(pRwObject);
            CObject::ResetAccessoryLighting();
        }
        else
        {
            CObject::Render();
        }
    }
    else
    {
        CCommon::RenderClumpOrAtomic(pRwObject);
    }

    if (CObject::m_bHasMaterial && CObject::m_bCanUpdateMaterial)
    {
        CObject::ProcessMaterialRestore();
    }

    CObject::m_bCanUpdateMaterial = true;
}

void CObject::ApplyMoveSpeed()
{
    if (m_pEntity)
    {
        float fTimeStep = CTimer::ms_fTimeStep;

        CMatrix mat;
        GetMatrix(&mat);
        mat.pos.x += fTimeStep * m_pEntity->vecMoveSpeed.x;
        mat.pos.y += fTimeStep * m_pEntity->vecMoveSpeed.y;
        mat.pos.z += fTimeStep * m_pEntity->vecMoveSpeed.z;
        UpdateMatrix(mat);
    }
}

float CObject::DistanceRemaining(CMatrix *matPos, CMatrix *m_matPositionTarget)
{
    float   fSX,fSY,fSZ;
    fSX = (matPos->pos.x - m_matPositionTarget->pos.x) * (matPos->pos.x - m_matPositionTarget->pos.x);
    fSY = (matPos->pos.y - m_matPositionTarget->pos.y) * (matPos->pos.y - m_matPositionTarget->pos.y);
    fSZ = (matPos->pos.z - m_matPositionTarget->pos.z) * (matPos->pos.z - m_matPositionTarget->pos.z);
    return sqrt(fSX + fSY + fSZ);
}

float CObject::RotationRemaining(CVector matPos, CVector m_vecRot)
{
    float fSX,fSY,fSZ;
    fSX = (matPos.x - m_vecRot.x) * (matPos.x - m_vecRot.x);
    fSY = (matPos.y - m_vecRot.y) * (matPos.y - m_vecRot.y);
    fSZ = (matPos.z - m_vecRot.z) * (matPos.z - m_vecRot.z);
    return sqrt(fSX + fSY + fSZ);
}

void CObject::MoveTo(float fX, float fY, float fZ, float fSpeed, float fRotX, float fRotY, float fRotZ)
{
    CMatrix mat;
    GetMatrix(&mat);

    if (CObject::m_byteMoving & 1)
    {
        CObject::StopMoving();
        mat.pos.x = CObject::m_matTarget.pos.x;
        mat.pos.y = CObject::m_matTarget.pos.y;
        mat.pos.z = CObject::m_matTarget.pos.z;

        if (CObject::m_bNeedRotate)
        {
            CObject::m_quatTarget.GetMatrix(&mat);
        }

        UpdateMatrix(mat);
    }

    CObject::m_dwMoveTick = CNecessary::GetTickCount();
    CObject::m_fMoveSpeed = fSpeed;
    CObject::m_matTarget.pos.x = fX;
    CObject::m_matTarget.pos.y = fY;
    CObject::m_matTarget.pos.z = fZ;
    CObject::m_byteMoving |= 1;

    if (fRotX <= -999.0f || fRotY <= -999.0f || fRotZ <= -999.0f)
    {
        CObject::m_bNeedRotate = false;
    }
    else
    {
        CObject::m_bNeedRotate = true;

        CObject::RotateMatrix(fRotX, fRotY, fRotZ);

        CMatrix matrix;
        GetMatrix(&matrix);
        m_quatStart.SetFromMatrix(matrix);
        m_quatTarget.SetFromMatrix(CObject::m_matTarget);
        m_quatStart.Normalize();
        m_quatTarget.Normalize();
    }

    CObject::m_fDistanceToTargetPoint = CObject::GetDistanceFromPoint(CObject::m_matTarget.pos);

    if (CMultiPlayer::GetActiveNetwork())
    {
        CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
        if (pPlayerPool)
        {
            // pPlayerPool->GetLocalPlayer()->ProcessSurfing(); // Surfing ยังไม่โยก
        }
    }

    ClearStaticWaitingForCollision();
}

void CObject::StopMoving()
{
    CVector vec;
    vec.x = 0.0f;
    vec.y = 0.0f;
    vec.z = 0.0f;

    SetMoveSpeedVector(vec);
    SetTurnSpeedVector(vec);

    CObject::m_byteMoving &= ~1;
}

void CObject::InstantRotate(float x, float y, float z)
{
    if (CUtil::GamePool_Object_GetAt(m_dwGTAId))
    {
        CScripting::ScriptCommand(&set_object_rotation, m_dwGTAId, x, y, z);
        CObject::m_vecRotation.x = x;
        CObject::m_vecRotation.y = y;
        CObject::m_vecRotation.z = z;
    }
}

void CObject::RotateMatrix(float x, float y, float z)
{
    CObject::m_vecRotation.x = x;
    CObject::m_vecRotation.y = y;
    CObject::m_vecRotation.z = z;

    x *= 0.017453292f;
    y *= 0.017453292f;
    z *= 0.017453292f;

    float cosx = cosf(x);
    float sinx = sinf(x);

    float cosy = cosf(y);
    float siny = sinf(y);

    float cosz = cosf(z);
    float sinz = sinf(z);

    float sinzx = sinz * sinx;
    float coszx = cosz * sinx;

    CObject::m_matTarget.right.x = cosz * cosy - sinzx * siny;
    CObject::m_matTarget.right.y = coszx * siny + sinz * cosy;
    CObject::m_matTarget.right.z = -(siny * cosx);

    CObject::m_matTarget.up.x = -(sinz * cosx);
    CObject::m_matTarget.up.y = cosz * cosx;
    CObject::m_matTarget.up.z = sinx;

    CObject::m_matTarget.at.x = sinzx * cosy + cosz * siny;
    CObject::m_matTarget.at.y = sinz * siny - coszx * cosy;
    CObject::m_matTarget.at.z = cosy * cosx;
}

void CObject::ProcessAttachedObject()
{
    if (CObject::m_attachedToObject != eInvalidId::INVALID_OBJECT_ID && !CObject::m_byteSyncRotation)
    {
        CMatrix targetMatrix;
        GetMatrix(&targetMatrix);

        CCommon::RwMatrixRotate(&targetMatrix, 0, CObject::m_vecAttachRotation.x);
        CCommon::RwMatrixRotate(&targetMatrix, 1, CObject::m_vecAttachRotation.y);
        CCommon::RwMatrixRotate(&targetMatrix, 2, CObject::m_vecAttachRotation.z);

        SetMatrix(targetMatrix);
        SetTurnSpeedVector(CVector());
        SetMoveSpeedVector(CVector());
    }
}

// void CObject::SetAttachedToVehicle(uint16_t vehicleId, CVector vecOrgAttachOffset, CVector vecOrgAttachRotation)
// {
//     if (vehicleId == eInvalidId::INVALID_OBJECT_ID)
//     {
//         CObject::m_attachedToVehicle = eInvalidId::INVALID_VEHICLE_ID;
//         CObject::m_vecAttachOffset = CVector();
//         CObject::m_vecAttachRotation = CVector();
//     }
//     else
//     {
//         CObject::m_attachedToVehicle = vehicleId;
//         CObject::m_vecAttachOffset.x = vecOrgAttachOffset.x;
//         CObject::m_vecAttachOffset.y = vecOrgAttachOffset.y;
//         CObject::m_vecAttachOffset.z = vecOrgAttachOffset.z;
//         CObject::m_vecAttachRotation.x = vecOrgAttachRotation.x;
//         CObject::m_vecAttachRotation.y = vecOrgAttachRotation.y;
//         CObject::m_vecAttachRotation.z = vecOrgAttachRotation.z;
//     }
// }

void CObject::SetAttachedToObject(uint16_t objectId, CVector vecOrgAttachOffset, CVector vecOrgAttachRotation, uint8_t byteSyncRotation)
{
    if (objectId == eInvalidId::INVALID_OBJECT_ID)
    {
        CObject::m_attachedToObject = eInvalidId::INVALID_OBJECT_ID;
        CObject::m_vecAttachOffset = CVector();
        CObject::m_vecAttachRotation = CVector();
        CObject::m_byteSyncRotation = 0;
    }
    else
    {
        CObject::m_attachedToObject = objectId;
        CObject::m_vecAttachOffset.x = vecOrgAttachOffset.x;
        CObject::m_vecAttachOffset.y = vecOrgAttachOffset.y;
        CObject::m_vecAttachOffset.z = vecOrgAttachOffset.z;
        CObject::m_vecAttachRotation.x = vecOrgAttachRotation.x;
        CObject::m_vecAttachRotation.y = vecOrgAttachRotation.y;
        CObject::m_vecAttachRotation.z = vecOrgAttachRotation.z;
        CObject::m_byteSyncRotation = byteSyncRotation;
    }
}

// void CObject::AttachToVehicle(CVehicle *pVehicle)
// {
//     if (pVehicle)
//     {
//         CVehiclePool* pVehiclePool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool();
//         if (!pVehiclePool) {
//             return;
//         }

//         if (!pVehiclePool->IsValidPointer(CObject::m_attachedToVehicle, pVehicle)) {
//             CNecessary::Log(OBFUSCATE("AttachToVehicle: pVehicle is no longer valid."));
//             return;
//         }

//         if (!CObject::IsObjectAttached(m_dwGTAId))
//         {
//             CScripting::ScriptCommand(&attach_object_to_car, m_dwGTAId, pVehicle->m_dwGTAId, CObject::m_vecAttachOffset.x, CObject::m_vecAttachOffset.y, CObject::m_vecAttachOffset.z, CObject::m_vecAttachRotation.x, CObject::m_vecAttachRotation.y, CObject::m_vecAttachRotation.z);
//         }
//     }
// }

void CObject::AttachToObject(CObject *pObject)
{
    if (pObject)
    {
        if (!CObject::IsObjectAttached(m_dwGTAId))
        {
            CScripting::ScriptCommand(&attach_object_to_object, m_dwGTAId, pObject->m_dwGTAId, CObject::m_vecAttachOffset.x, CObject::m_vecAttachOffset.y, CObject::m_vecAttachOffset.z, CObject::m_vecAttachRotation.x, CObject::m_vecAttachRotation.y, CObject::m_vecAttachRotation.z);
        }
    }
}

void CObject::SetMaterial(int iModel, uint8_t byteMaterialIndex, char* txdname, char* texturename, uint32_t dwColor)
{
    if (byteMaterialIndex >= MAX_MATERIALS_PER_MODEL) {
        return;
    }

    auto& material = CObject::m_Material[byteMaterialIndex];

    if (iModel <= 0 || CCommon::IsExistForModelInfo(iModel))
    {
        uint32_t dwConvertedColor = dwColor;

        if (iModel == eInvalidId::INVALID_ID)
        {
            if (dwColor == 0xFF000000)
            {
                dwConvertedColor = __builtin_bswap32(dwColor);
            }
        }

        if (material.bTextureWasCreated && material.pMaterialTexture)
        {
            CSprite2d::Destructor(material.pMaterialTexture);
            material.pMaterialTexture = nullptr;
            material.bTextureWasCreated = false;
        }

        if (iModel <= 0)
        {
            if (!iModel)
            {
                if (!CObject::m_bLankTexture)
                {
                    CObject::m_bLankTexture = CTxdManager::LoadTextureFromTxd(OBFUSCATE("samp"), OBFUSCATE("blanktex"));
                }

                material.pMaterialTexture = CObject::m_bLankTexture;
            }
        }
        else
        {
            int waitCounter = 0;
            if (!CStreaming::IsModelLoaded(iModel))
            {
                CStreaming::RequestModel(iModel);
                CStreaming::LoadAllRequestedModels();

                while (!CStreaming::IsModelLoaded(iModel))
                {
                    usleep(2000);
                    waitCounter++;
                    if (waitCounter > 250)
                    {
                        material.pMaterialTexture = nullptr;
                        break;
                    }
                }
            }

            if (CStreaming::IsModelLoaded(iModel))
            {
                material.pMaterialTexture = CTxdManager::LoadTextureFromTxd(txdname, texturename);

                if (!CCommon::ModelInfoGetReferenceCount(iModel))
                {
                    CStreaming::RemoveModel(iModel, false);
                }
            }
        }

        material.dwMaterialColor = dwConvertedColor;
        material.iType = eMaterialType::MATERIAL_TYPE_TEXTURE;
        material.bTextureWasCreated = true;
        CObject::m_bHasMaterial = true;
    }
}

void CObject::SetMaterialText(uint8_t byteMaterialIndex, uint8_t byteMaterialSize, const char* szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char* szText)
{
    if (byteMaterialIndex >= 16) {
        return;
    }

    if (!szText || !CNecessary::MPSTRLEN(szText)) {
        return;
    }

    auto& material = CObject::m_Material[byteMaterialIndex];

    if (material.bTextureWasCreated && material.pMaterialTexture)
    {
        CSprite2d::Destructor(material.pMaterialTexture);
        material.pMaterialTexture = nullptr;
    }

    material.TextInfo.byteMaterialSize = byteMaterialSize;

    memset(material.TextInfo.szFontName, 0, 32);
    strncpy(material.TextInfo.szFontName, szFontName, 32);
    material.TextInfo.szFontName[31] = '\0';

    material.TextInfo.byteFontSize = byteFontSize;
    material.TextInfo.byteFontBold = byteFontBold;
    material.TextInfo.dwFontColor = dwFontColor;
    material.TextInfo.dwBackgroundColor = dwBackgroundColor;
    material.TextInfo.byteAlign = byteAlign;

    memset(material.TextInfo.szText, 0, 2048);
    strncpy(material.TextInfo.szText, szText, 2048);
    material.TextInfo.szText[2047] = '\0';

    material.dwMaterialColor = 0;
    material.iType = eMaterialType::MATERIAL_TYPE_TEXT;
    material.bTextureWasCreated = false;

    CObject::m_bHasMaterial = true;
}

void CObject::CreateMaterialFontTexture()
{
    for (uint8_t i = 0; i < MAX_MATERIALS_PER_MODEL; ++i)
    {
        auto& material = CObject::m_Material[i];

        if (material.bTextureWasCreated || material.iType != eMaterialType::MATERIAL_TYPE_TEXT) {
            continue;
        }

        if (material.TextInfo.szText[0] == '\0' || !CNecessary::MPSTRLEN(material.TextInfo.szText)) {
            continue;
        }

        int iSizeX = 0;
        int iSizeY = 0;

        if (CObject::ConvertMaterialTextSize(material.TextInfo.byteMaterialSize, &iSizeX, &iSizeY))
        {
            if (material.TextInfo.byteAlign < eMaterialTextAlign::MATERIAL_ALIGN_LEFT || material.TextInfo.byteAlign > eMaterialTextAlign::MATERIAL_ALIGN_RIGHT)
            {
                material.TextInfo.byteAlign = eMaterialTextAlign::MATERIAL_ALIGN_CENTER;
            }

            uint8_t byteScaledFontSize = static_cast<uint8_t>(material.TextInfo.byteFontSize * 0.75f);
            if (byteScaledFontSize == 0 && material.TextInfo.byteFontSize > 0)
            {
                byteScaledFontSize = 1;
            }

            material.pMaterialTexture = CGame::GetMaterialTextGenerator()->Create(iSizeX, iSizeY, material.TextInfo.szFontName, byteScaledFontSize, material.TextInfo.byteFontBold, material.TextInfo.dwFontColor, material.TextInfo.dwBackgroundColor, material.TextInfo.byteAlign, material.TextInfo.szText);

            if (material.pMaterialTexture)
            {
                material.bTextureWasCreated = true;
            }
        }
    }
}

bool CObject::ConvertMaterialTextSize(int iMatSize, int *iSizeX, int *iSizeY)
{
    if (!iSizeX || !iSizeY) {
        return false;
    }

    switch (iMatSize)
    {
        case eMaterialSize::MATERIAL_SIZE_32X32: {
            *iSizeX = 32;
            *iSizeY = 32;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_64X32: {
            *iSizeX = 64;
            *iSizeY = 32;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_64X64: {
            *iSizeX = 64;
            *iSizeY = 64;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_128X32: {
            *iSizeX = 128;
            *iSizeY = 32;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_128X64: {
            *iSizeX = 128;
            *iSizeY = 64;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_128X128: {
            *iSizeX = 128;
            *iSizeY = 128;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_256X32: {
            *iSizeX = 256;
            *iSizeY = 32;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_256X64: {
            *iSizeX = 256;
            *iSizeY = 64;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_256X128: {
            *iSizeX = 256;
            *iSizeY = 128;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_256X256: {
            *iSizeX = 256;
            *iSizeY = 256;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_512X64: {
            *iSizeX = 512;
            *iSizeY = 64;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_512X128: {
            *iSizeX = 512;
            *iSizeY = 128;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_512X256: {
            *iSizeX = 512;
            *iSizeY = 256;
            return true;
        }

        case eMaterialSize::MATERIAL_SIZE_512X512: {
            *iSizeX = 512;
            *iSizeY = 512;
            return true;
        }

        default: {
            *iSizeX = 0;
            *iSizeY = 0;
            return false;
        }
    }
}

RpAtomic* ApplyCustomMaterialsCallback(RpAtomic* rpAtomic, CObject* pObject)
{
    if (*(uint8_t*)rpAtomic == rpATOMIC)
    {
        RpGeometry* geometry = rpAtomic->geometry;
        if (geometry && pObject)
        {
            int iTotalEntries = geometry->matList.numMaterials;
            if (iTotalEntries > MAX_MATERIALS_PER_MODEL)
            {
                iTotalEntries = MAX_MATERIALS_PER_MODEL;
            }

            if (iTotalEntries > 0)
            {
                RpMaterial** pRwMaterial = geometry->matList.materials;
                for (int i = 0; i < iTotalEntries; i++)
                {
                    if (pObject->m_Material[i].pMaterialTexture)
                    {
                        pRwMaterial[i]->texture = pObject->m_Material[i].pMaterialTexture;
                        pObject->m_bMaterialTextureChanged = true;
                    }

                    if (pObject->m_Material[i].dwMaterialColor)
                    {
                        geometry->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;
                        geometry->flags &= ~rpGEOMETRYPRELIT;

                        pRwMaterial[i]->color = *(RwRGBA*)&pObject->m_Material[i].dwMaterialColor;
                        pRwMaterial[i]->surfaceProps.ambient = 1.0f;
                        pRwMaterial[i]->surfaceProps.specular = 0.0f;
                        pRwMaterial[i]->surfaceProps.diffuse = 1.0f;

                        pObject->m_bMaterialColorChanged = true;
                    }
                }
            }
        }
    }

    return rpAtomic;
}

RpAtomic* BackupOriginalMaterialsCallback(RpAtomic* rpAtomic, CObject* pObject)
{
    if (*(uint8_t*)rpAtomic == rpATOMIC)
    {
        RpGeometry* geometry = rpAtomic->geometry;
        if (geometry && pObject)
        {
            pObject->m_Material[0].originalGeometryFlags = geometry->flags;

            int iTotalEntries = geometry->matList.numMaterials;
            if (iTotalEntries > MAX_MATERIALS_PER_MODEL)
            {
                iTotalEntries = MAX_MATERIALS_PER_MODEL;
            }

            if (iTotalEntries > 0)
            {
                RpMaterial** pRwMaterial = geometry->matList.materials;
                for (int i = 0; i < iTotalEntries; i++)
                {
                    pObject->m_Material[i].originalMaterialTexture = pRwMaterial[i]->texture;
                    pObject->m_Material[i].originalMaterialColor = *(RwRGBA*)&pRwMaterial[i]->color;
                }
            }
        }
    }

    return rpAtomic;
}

RpAtomic* RestoreDefaultMaterialsCallback(RpAtomic* rpAtomic, CObject* pObject)
{
    if (*(uint8_t*)rpAtomic == rpATOMIC)
    {
        RpGeometry* geometry = rpAtomic->geometry;
        if (geometry && pObject)
        {
            int iTotalEntries = geometry->matList.numMaterials;
            if (iTotalEntries > MAX_MATERIALS_PER_MODEL)
            {
                iTotalEntries = MAX_MATERIALS_PER_MODEL;
            }

            if (iTotalEntries > 0)
            {
                geometry->flags = pObject->m_Material[0].originalGeometryFlags;

                RpMaterial** pRwMaterial = geometry->matList.materials;
                for (int i = 0; i < iTotalEntries; i++)
                {
                    if (pObject->m_bMaterialTextureChanged)
                    {
                        pRwMaterial[i]->texture = pObject->m_Material[i].originalMaterialTexture;
                    }

                    if (pObject->m_bMaterialColorChanged)
                    {
                        pRwMaterial[i]->color = pObject->m_Material[i].originalMaterialColor;
                    }
                }

                pObject->m_bMaterialTextureChanged = false;
                pObject->m_bMaterialColorChanged = false;
            }
        }
    }

    return rpAtomic;
}

void CObject::ProcessMaterialUpdates()
{
    if (CObject::m_bHasMaterial && CObject::m_bCanUpdateMaterial)
    {
        CObject::HandleCustomMaterialUpdate();

        if (CObject::m_bCanChangeAccessoryMaterial)
        {
            CObject::SetupAccessoryLighting();
        }
    }

    CObject::m_bCanUpdateMaterial = true;
}

void CObject::HandleCustomMaterialUpdate()
{
    if (!CObject::m_bHasMaterial || !m_pEntity) {
        return;
    }

    if (m_pEntity->m_pRwObject)
    {
        if (CObject::m_pLastMaterialRwObject == m_pEntity->m_pRwObject)
        {
            RwFrame* pParent = reinterpret_cast<RwFrame*>(m_pEntity->m_pRwObject->parent);
            if (pParent)
            {
                RwFrameForAllObjects(pParent, reinterpret_cast<RwObjectCallBack>(ApplyCustomMaterialsCallback), this);
            }
        }
        else
        {
            RwFrame* pParent = reinterpret_cast<RwFrame*>(m_pEntity->m_pRwObject->parent);
            if (pParent)
            {
                RwFrameForAllObjects(pParent, reinterpret_cast<RwObjectCallBack>(BackupOriginalMaterialsCallback), this);
            }

            CObject::m_pLastMaterialRwObject = m_pEntity->m_pRwObject;
            CObject::m_bCanUpdateMaterial = false;
        }
    }
}

void CObject::ProcessMaterialRestore()
{
    if (!CObject::m_bHasMaterial || !m_pEntity) {
        return;
    }

    if (CObject::m_bCanChangeAccessoryMaterial)
    {
        CObject::ResetAccessoryLighting();
    }

    if (CObject::m_bMaterialTextureChanged || CObject::m_bMaterialColorChanged)
    {
        if (m_pEntity->m_pRwObject && CObject::m_pLastMaterialRwObject == m_pEntity->m_pRwObject)
        {
            RwFrame* pParent = reinterpret_cast<RwFrame*>(m_pEntity->m_pRwObject->parent);
            if (pParent)
            {
                RwFrameForAllObjects(pParent, reinterpret_cast<RwObjectCallBack>(RestoreDefaultMaterialsCallback), this);
            }
        }
    }
}

void CObject::SetupAccessoryLighting()
{
    if (m_pEntity && !CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        m_pEntity->m_nEntityFlags |= 0x10000000;
        reinterpret_cast<void (*)(CEntityGta*)>(*reinterpret_cast<void**>(m_pEntity->vtable + 0xA0))(m_pEntity); // CEntity::SetupLighting
        CObject::m_bAccessoryMaterialChanged = true;
    }
}

void CObject::ResetAccessoryLighting()
{
    if (m_pEntity && !CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        if (CObject::m_bAccessoryMaterialChanged)
        {
            reinterpret_cast<void (*)(CEntityGta*, int)>(*reinterpret_cast<void**>(m_pEntity->vtable + 0xA8))(m_pEntity, 1); // CEntity::RemoveLighting
            CObject::m_bAccessoryMaterialChanged = false;
        }
    }
}

uint32_t CObject::IsObjectAttached(uint32_t iID)
{
    return CScripting::ScriptCommand(&is_object_attached, iID);
}

bool CObject::AttachedToMovingEntity()
{
    if (CObject::m_attachedToObject == eInvalidId::INVALID_OBJECT_ID)
    {
        if (CObject::m_attachedToVehicle != eInvalidId::INVALID_VEHICLE_ID) {
            return true;
        }

        return (CObject::m_byteMoving & 1);
    }
    else
    {
        if (CObject::m_attachedToObject >= 0 && CObject::m_attachedToObject < eMaxItem::MAX_OBJECTS)
        {
            if (CMultiPlayer::GetActiveNetwork())
            {
                CObjectPool* pObjectPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetObjectPool();
                if (pObjectPool)
                {
                    CObject* pObject = pObjectPool->GetObject(CObject::m_attachedToObject);
                    if (pObject)
                    {
                        return (pObject->m_byteMoving & 1);
                    }
                }
            }
        }
    }

    return false;
}

void CObject::Render()
{
    if (!m_pEntity) {
        return;
    }

    if (!CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        reinterpret_cast<void (*)(CEntityGta*)>(*reinterpret_cast<void**>(m_pEntity->vtable + 0x90))(m_pEntity); // CEntity::PreRender();
        reinterpret_cast<void (*)(CEntityGta*)>(SA_ADDR(0x16FB9C) + _dyld_get_image_vmaddr_slide(0))(m_pEntity); // CRenderer::RenderOneNonRoad
    }
}

void CObject::SetFlags()
{
    if (!m_pEntity) {
        return;
    }

    if (!CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        m_pEntity->m_nEntityFlags |= 0x80000000;
    }
}

void CObject::UpdateRenderPosition()
{
    if (!m_pEntity) {
        return;
    }

    if (!CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        CMatrix entityMatrix;
        CVector colSphereCenter;

        GetMatrix(&entityMatrix);
        int modelIndex = m_pEntity->nModelIndex;

        if (CCommon::ModelInfoGetColSphereVecCenter(modelIndex, &colSphereCenter))
        {
            CCommon::ProjectMatrix(&m_vecSpawnPos, &entityMatrix, &colSphereCenter);
        }
        else
        {
            CObject::m_vecSpawnPos.x = entityMatrix.pos.x;
            CObject::m_vecSpawnPos.y = entityMatrix.pos.y;
            CObject::m_vecSpawnPos.z = entityMatrix.pos.z;
        }
    }
}

RpAtomic* CObject::FixObjectLightingCB(RpAtomic* rpAtomic, [[maybe_unused]] void* data)
{
    if (rpAtomic && *(uint8_t*)rpAtomic == rpATOMIC)
    {
        RpGeometry* geometry = rpAtomic->geometry;
        if (geometry)
        {
            int numMaterials = geometry->matList.numMaterials;
            RpMaterial** materials = geometry->matList.materials;
            if (materials && numMaterials > 0)
            {
                bool hasBugMaterial = false;

                for (int i = 0; i < numMaterials; i++)
                {
                    if (materials[i])
                    {
                        // แก้ไข Object ที่รับแสงอาทิตย์หรือแสงไฟส่องทิศทางไม่ได้ ทำให้บัค Material สีดำ
                        if (materials[i]->surfaceProps.diffuse <= 0.00f)
                        {
                            hasBugMaterial = true;
                            break;
                        }
                    }
                }

                if (hasBugMaterial)
                {
                    if (geometry->flags & rpGEOMETRYPRELIT)
                    {
                        geometry->flags &= ~rpGEOMETRYPRELIT;
                        geometry->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;
                    }

                    for (int i = 0; i < numMaterials; i++)
                    {
                        if (materials[i])
                        {
                            materials[i]->surfaceProps.diffuse = 1.0f;
                        }
                    }

                    if (geometry->repEntry)
                    {
                        RwResourcesFreeResEntry(geometry->repEntry);
                        geometry->repEntry = nullptr;
                    }
                }
            }
        }
    }

    return rpAtomic;
}

void CObject::ModifyGeometry(RpGeometry* geometry, std::vector<SavedGeometry>& saved, bool bIsObjectModel)
{
    if (!geometry) {
        return;
    }

    for (const auto& item : saved)
    {
        if (item.geometry == geometry)
        {
            return;
        }
    }

    SavedGeometry savedItem;
    savedItem.geometry = geometry;
    savedItem.flags = geometry->flags;
    savedItem.allocatedPrelit = false;
    savedItem.originalPrelitPtr = geometry->preLitLum;

    // สำหรับวัตถุทั่วไป (bIsObjectModel = true) จะบังคับสีขาวสว่างผ่าน preLitLum และสลับ Flags เพื่อปิดแสงกล้อง/แสงเกม
    // สำหรับอาวุธ/ยานพาหนะ จะไม่สร้าง prelit และจะไม่สลับ Flags (ปล่อยให้ใช้แสงกล้องสะท้อนธรรมชาติ)
    if (bIsObjectModel && geometry->numVertices > 0)
    {
        if (geometry->preLitLum)
        {
            savedItem.originalPrelitVal.assign(geometry->preLitLum, geometry->preLitLum + geometry->numVertices);
        }
        else
        {
            geometry->preLitLum = (RwRGBA*)malloc(geometry->numVertices * sizeof(RwRGBA));
            savedItem.allocatedPrelit = true;
        }

        for (int i = 0; i < geometry->numVertices; ++i)
        {
            geometry->preLitLum[i].red = 255;
            geometry->preLitLum[i].green = 255;
            geometry->preLitLum[i].blue = 255;
            geometry->preLitLum[i].alpha = 255;
        }
    }

    if (geometry->matList.materials)
    {
        for (int i = 0; i < geometry->matList.numMaterials; ++i)
        {
            RpMaterial* mat = geometry->matList.materials[i];
            if (mat)
            {
                savedItem.savedMaterials.push_back({ mat, mat->color });
                mat->color.red = 255;
                mat->color.green = 255;
                mat->color.blue = 255;
                mat->color.alpha = 255;
            }
        }
    }

    saved.push_back(savedItem);

    if (bIsObjectModel)
    {
        geometry->flags |= 0x00000008;  // rpGEOMETRYPRELIT
        geometry->flags &= ~0x00000020; // rpGEOMETRYLIGHT
    }

    if (geometry->repEntry)
    {
        RwResourcesFreeResEntry(geometry->repEntry);
        geometry->repEntry = nullptr;
    }
}

RpAtomic* CObject::ModifyGeometryCB(RpAtomic* rpAtomic, void* data)
{
    ModifyParams* params = reinterpret_cast<ModifyParams*>(data);
    if (rpAtomic && rpAtomic->geometry && params)
    {
        CObject::ModifyGeometry(rpAtomic->geometry, params->saved, params->bIsObjectModel);
    }

    return rpAtomic;
}

void CObject::ProcessObjectGeometries(uintptr_t pRwObject, std::vector<SavedGeometry>& saved, bool bIsObjectModel)
{
    if (!pRwObject) {
        return;
    }

    uint8_t type = *reinterpret_cast<uint8_t*>(pRwObject);

    ModifyParams params = { saved, bIsObjectModel };

    if (type == rpCLUMP) // RpClump
    {
        RpClump* clump = reinterpret_cast<RpClump*>(pRwObject);
        CUtil::RpClumpForAllAtomics(clump, CObject::ModifyGeometryCB, &params);
    }
    else if (type == rpATOMIC) // RpAtomic
    {
        RpAtomic* atomic = reinterpret_cast<RpAtomic*>(pRwObject);
        CObject::ModifyGeometryCB(atomic, &params);
    }
}

void CObject::RestoreObjectGeometries(const std::vector<SavedGeometry>& saved)
{
    for (const auto& item : saved)
    {
        item.geometry->flags = item.flags;
        if (item.allocatedPrelit)
        {
            free(item.geometry->preLitLum);
            item.geometry->preLitLum = item.originalPrelitPtr;
        }
        else if (!item.originalPrelitVal.empty() && item.geometry->preLitLum)
        {
            std::copy(item.originalPrelitVal.begin(), item.originalPrelitVal.end(), item.geometry->preLitLum);
        }

        for (const auto& matSave : item.savedMaterials)
        {
            matSave.material->color = matSave.color;
        }

        if (item.geometry->repEntry)
        {
            RwResourcesFreeResEntry(item.geometry->repEntry);
            item.geometry->repEntry = nullptr;
        }
    }
}

void CObject::SetDirectModelCreation(bool enable)
{
    directModelCreation = enable;
}

extern "C"
{
    __attribute__((visibility("hidden"))) uintptr_t CObject_CreateObjectModelReturn = SA_ADDR(0xF9CD4) + _dyld_get_image_vmaddr_slide(0);

    __attribute__((visibility("hidden"))) int32_t CObject_ResolveCreateModel(int32_t model)
    {
        if ((model < 0) != directModelCreation)
        {
            int32_t index = static_cast<int32_t>(0u - static_cast<uint32_t>(model));
            return *reinterpret_cast<int32_t*>(SA_ADDR(0x71F3C4) + _dyld_get_image_vmaddr_slide(0) + static_cast<int64_t>(index) * 0x1C + 0x18);
        }

        return model;
    }
}

extern "C" __attribute__((naked, visibility("default"))) void CObject_CreateObjectModel_hook()
{
    __asm__ volatile(
        "sub sp, sp, #0x310\n"
        "stp x0, x1, [sp, #0x0]\n"
        "stp x2, x3, [sp, #0x10]\n"
        "stp x4, x5, [sp, #0x20]\n"
        "stp x6, x7, [sp, #0x30]\n"
        "stp x8, x9, [sp, #0x40]\n"
        "stp x10, x11, [sp, #0x50]\n"
        "stp x12, x13, [sp, #0x60]\n"
        "stp x14, x15, [sp, #0x70]\n"
        "stp x16, x17, [sp, #0x80]\n"
        "stp x18, x19, [sp, #0x90]\n"
        "stp x20, x21, [sp, #0xA0]\n"
        "stp x22, x23, [sp, #0xB0]\n"
        "stp x24, x25, [sp, #0xC0]\n"
        "stp x26, x27, [sp, #0xD0]\n"
        "stp x28, x29, [sp, #0xE0]\n"
        "str x30, [sp, #0xF0]\n"
        "mrs x9, nzcv\n"
        "str x9, [sp, #0xF8]\n"
        "stp q0, q1, [sp, #0x100]\n"
        "stp q2, q3, [sp, #0x120]\n"
        "stp q4, q5, [sp, #0x140]\n"
        "stp q6, q7, [sp, #0x160]\n"
        "stp q8, q9, [sp, #0x180]\n"
        "stp q10, q11, [sp, #0x1A0]\n"
        "stp q12, q13, [sp, #0x1C0]\n"
        "stp q14, q15, [sp, #0x1E0]\n"
        "stp q16, q17, [sp, #0x200]\n"
        "stp q18, q19, [sp, #0x220]\n"
        "stp q20, q21, [sp, #0x240]\n"
        "stp q22, q23, [sp, #0x260]\n"
        "stp q24, q25, [sp, #0x280]\n"
        "stp q26, q27, [sp, #0x2A0]\n"
        "stp q28, q29, [sp, #0x2C0]\n"
        "stp q30, q31, [sp, #0x2E0]\n"
        "bl _CObject_ResolveCreateModel\n"
        "str w0, [sp]\n"
        "ldp q0, q1, [sp, #0x100]\n"
        "ldp q2, q3, [sp, #0x120]\n"
        "ldp q4, q5, [sp, #0x140]\n"
        "ldp q6, q7, [sp, #0x160]\n"
        "ldp q8, q9, [sp, #0x180]\n"
        "ldp q10, q11, [sp, #0x1A0]\n"
        "ldp q12, q13, [sp, #0x1C0]\n"
        "ldp q14, q15, [sp, #0x1E0]\n"
        "ldp q16, q17, [sp, #0x200]\n"
        "ldp q18, q19, [sp, #0x220]\n"
        "ldp q20, q21, [sp, #0x240]\n"
        "ldp q22, q23, [sp, #0x260]\n"
        "ldp q24, q25, [sp, #0x280]\n"
        "ldp q26, q27, [sp, #0x2A0]\n"
        "ldp q28, q29, [sp, #0x2C0]\n"
        "ldp q30, q31, [sp, #0x2E0]\n"
        "ldr x9, [sp, #0xF8]\n"
        "msr nzcv, x9\n"
        "ldr x30, [sp, #0xF0]\n"
        "ldp x2, x3, [sp, #0x10]\n"
        "ldp x4, x5, [sp, #0x20]\n"
        "ldp x6, x7, [sp, #0x30]\n"
        "ldp x8, x9, [sp, #0x40]\n"
        "ldp x10, x11, [sp, #0x50]\n"
        "ldp x12, x13, [sp, #0x60]\n"
        "ldp x14, x15, [sp, #0x70]\n"
        "ldp x16, x17, [sp, #0x80]\n"
        "ldp x18, x19, [sp, #0x90]\n"
        "ldp x20, x21, [sp, #0xA0]\n"
        "ldp x22, x23, [sp, #0xB0]\n"
        "ldp x24, x25, [sp, #0xC0]\n"
        "ldp x26, x27, [sp, #0xD0]\n"
        "ldp x28, x29, [sp, #0xE0]\n"
        "ldp x0, x1, [sp]\n"
        "add sp, sp, #0x310\n"
        "adrp x16, _CObject_CreateObjectModelReturn@PAGE\n"
        "ldr x16, [x16, _CObject_CreateObjectModelReturn@PAGEOFF]\n"
        "br x16\n"
    );
}
