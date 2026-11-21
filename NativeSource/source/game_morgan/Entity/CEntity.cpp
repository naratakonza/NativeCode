#include "CEntity.h"
#include "../CGame.h"
#include "../CWorld.h"
#include "../CCommon.h"
#include "../CStreaming.h"
#include "../RenderWare/rwcore.h"
#include "../Models/CBaseModelInfo.h"
#include <algorithm>
#include <string>

#include "../Multiplayer/Guis/CGui.h"
#include "../Multiplayer/CMultiPlayer.h"
#include "../Enums/eSpecialVehicle.h"
#include "../Models/CModelInfo.h"
#include "../Scripts/CScripting.h"
#include "../../utils/Necessary/CNecessary.h"

#include <mach-o/dyld.h>
#include <pthread.h>
#include <unistd.h>

void CEntity::Add()
{
    if (!m_pEntity || CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    if (!m_pEntity->dwUnkModelRel)
    {
        CVector vec = CVector();
        SetMoveSpeedVector(vec);
        SetTurnSpeedVector(vec);

        CWorld::AddEntity(m_pEntity);

        CMatrix mat;
        GetMatrix(&mat);
        TeleportTo(mat.pos.x, mat.pos.y, mat.pos.z);
    }
}

void CEntity::Remove()
{
    if (!m_pEntity || CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    if (m_pEntity->dwUnkModelRel)
    {
        CWorld::RemoveEntity(m_pEntity);
    }
}

void CEntity::Render()
{
    if (!m_pEntity) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x16FB9C), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x316F14), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text");
    if (!verified || !pthread_main_np()) {
        return;
    }

    uintptr_t pRwObject = GetRWObject();

    int iModel = GetModelIndex();
    if (iModel >= 400 && iModel <= 611 && pRwObject) {
        // CVisibilityPlugins::SetupVehicleVariables
        reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x316F14) + _dyld_get_image_vmaddr_slide(0))(pRwObject);
    }

    // m_pEntity->PreRender();
    reinterpret_cast<void (*)(CEntityGta*)>(*reinterpret_cast<void**>(m_pEntity->vtable + 0x90))(m_pEntity);

    if (iModel < 400 || iModel > 611)
    {
        if (!pRwObject)
        {
            return;
        }
    }
    else if (!pRwObject)
    {
        return;
    }

    // CRenderer::RenderOneNonRoad
    reinterpret_cast<void (*)(CEntityGta*)>(SA_ADDR(0x16FB9C) + _dyld_get_image_vmaddr_slide(0))(m_pEntity);
}

void CEntity::SetGravityProcessing(bool bProcess)
{
    if (!m_pEntity || CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    if (bProcess)
    {
        m_pEntity->m_nEntityFlags &= 0x7FFFFFFDu;
    }
    else
    {
        m_pEntity->m_nEntityFlags |= 0x80000002u;
    }
}

void CEntity::SetCollisionChecking(bool bCheck)
{
    if (m_pEntity && !CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        m_pEntity->nEntityFlags.m_bUsesCollision = bCheck;
    }
}

bool CEntity::GetCollisionChecking()
{
    if (!m_pEntity)
    {
        return true;
    }

    if (CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        return true;
    }

    return m_pEntity->nEntityFlags.m_bCollisionProcessed;
}

bool CEntity::IsAdded()
{
    return m_pEntity && !CCommon::IsGameEntityArePlaceable(m_pEntity) && m_pEntity->dwUnkModelRel != 0;
}

void CEntity::SetMoveSpeedVector(CVector Vector)
{
	if (!IsAdded()) {
		return;
	}

	if (!m_pEntity) {
		return;
	}

	m_pEntity->vecMoveSpeed.x = Vector.x;
	m_pEntity->vecMoveSpeed.y = Vector.y;
	m_pEntity->vecMoveSpeed.z = Vector.z;
}

void CEntity::GetMoveSpeedVector(CVector* Vector)
{
	if (!IsAdded()) {
		*Vector = CVector();
		return;
	}

	if (!m_pEntity) {
		return;
	}

	Vector->x = m_pEntity->vecMoveSpeed.x;
	Vector->y = m_pEntity->vecMoveSpeed.y;
	Vector->z = m_pEntity->vecMoveSpeed.z;
}

void CEntity::SetTurnSpeedVector(CVector Vector)
{
	if (!IsAdded()) {
		return;
	}

	if (!m_pEntity) {
		return;
	}

	m_pEntity->m_vecTurnSpeed.x = Vector.x;
	m_pEntity->m_vecTurnSpeed.y = Vector.y;
	m_pEntity->m_vecTurnSpeed.z = Vector.z;
}

void CEntity::GetTurnSpeedVector(CVector* Vector)
{
	if (!IsAdded()) {
		*Vector = CVector();
		return;
	}

	if (!m_pEntity) {
		return;
	}

	Vector->x = m_pEntity->m_vecTurnSpeed.x;
	Vector->y = m_pEntity->m_vecTurnSpeed.y;
	Vector->z = m_pEntity->m_vecTurnSpeed.z;
}

void CEntity::TeleportTo(float x, float y, float z)
{
    if (!pthread_main_np() || !m_pEntity || !m_pEntity->vtable || CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    if (!m_pEntity->mat) {
        return;
    }

    const uint16_t modelIndex = m_pEntity->nModelIndex;
    if (modelIndex == eSpecialVehicle::TRAIN_PASSENGER_LOCO || modelIndex == eSpecialVehicle::TRAIN_FREIGHT_LOCO || modelIndex == eSpecialVehicle::TRAIN_TRAM)
    {
        CScripting::ScriptCommand(&put_train_at, m_dwGTAId, x, y, z);
    }
    else
    {
        const auto teleportEntity = *reinterpret_cast<void (*const *)(CEntityGta*, CVector, uint8_t)>(m_pEntity->vtable + 0x78);
        teleportEntity(m_pEntity, {x, y, z}, 0);
    }
}

void CEntity::SetPosition(CVector vecPos)
{
	if (!m_pEntity || !m_pEntity->mat) {
		return;
	}

	m_pEntity->mat->pos = vecPos;
}

void CEntity::GetPosition(CVector* vecRet)
{
	if (!m_pEntity || !m_pEntity->mat) {
		*vecRet = CVector();
		return;
	}

	*vecRet = m_pEntity->mat->pos;
}

void CEntity::SetMatrix(CMatrix Matrix, [[maybe_unused]] bool checkValid)
{
	if (!m_pEntity || !m_pEntity->mat) {
		return;
	}

	m_pEntity->mat->right.x = Matrix.right.x;
	m_pEntity->mat->right.y = Matrix.right.y;
	m_pEntity->mat->right.z = Matrix.right.z;

	m_pEntity->mat->up.x = Matrix.up.x;
	m_pEntity->mat->up.y = Matrix.up.y;
	m_pEntity->mat->up.z = Matrix.up.z;

	m_pEntity->mat->at.x = Matrix.at.x;
	m_pEntity->mat->at.y = Matrix.at.y;
	m_pEntity->mat->at.z = Matrix.at.z;

	m_pEntity->mat->pos.x = Matrix.pos.x;
	m_pEntity->mat->pos.y = Matrix.pos.y;
	m_pEntity->mat->pos.z = Matrix.pos.z;
}

void CEntity::GetMatrix(CMatrix* Matrix)
{
    if (!m_pEntity || !m_pEntity->mat) {
        CNecessary::Log(OBFUSCATE("GetMatrix: Entity is null"));

		Matrix->right.x = 0.0f;
		Matrix->right.y = 0.0f;
		Matrix->right.z = 0.0f;

		Matrix->up.x = 0.0f;
		Matrix->up.y = 0.0f;
		Matrix->up.z = 0.0f;

		Matrix->at.x = 0.0f;
		Matrix->at.y = 0.0f;
		Matrix->at.z = 0.0f;

		Matrix->pos.x = 0.0f;
		Matrix->pos.y = 0.0f;
		Matrix->pos.z = 0.0f;
        return;
    }

	Matrix->right.x = m_pEntity->mat->right.x;
	Matrix->right.y = m_pEntity->mat->right.y;
	Matrix->right.z = m_pEntity->mat->right.z;

	Matrix->up.x = m_pEntity->mat->up.x;
	Matrix->up.y = m_pEntity->mat->up.y;
	Matrix->up.z = m_pEntity->mat->up.z;

	Matrix->at.x = m_pEntity->mat->at.x;
	Matrix->at.y = m_pEntity->mat->at.y;
	Matrix->at.z = m_pEntity->mat->at.z;

	Matrix->pos.x = m_pEntity->mat->pos.x;
	Matrix->pos.y = m_pEntity->mat->pos.y;
	Matrix->pos.z = m_pEntity->mat->pos.z;
}

uint16_t CEntity::GetModelIndex()
{
	if (!m_pEntity) {
		return 0;
	}

	return m_pEntity->nModelIndex;
}

void CEntity::GetBoundCentre(CVector* centre)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x14318C), "\x0A\x64\x80\x79\x29\x79\x6A\xF8\x29\x19\x40\xF9", 12, "__text");
    if (!verified) {
        return;
    }

    reinterpret_cast<void (*)(CEntityGta*, CVector*)>(SA_ADDR(0x14316C) + _dyld_get_image_vmaddr_slide(0))(m_pEntity, centre);
}

float CEntity::GetDistanceFromCamera()
{
    const auto slide = _dyld_get_image_vmaddr_slide(0);
    if (!m_pEntity || m_pEntity->vtable == SA_ADDR(0x49BD50) + slide) {
        return 100000.0f;
    }

    const auto& pos = m_pEntity->mat->pos;
    const auto& camPos = *reinterpret_cast<const CVector*>(SA_ADDR(0x72D8A8) + slide); // TheCamera->m_mCameraMatrix.pos

    const float deltaX = pos.x - camPos.x;
    const float deltaY = pos.y - camPos.y;
    const float deltaZ = pos.z - camPos.z;

    return sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
}

bool CEntity::IsPointInsideEntity(CVector* pointStart, CVector* pointEnd, CVector* pOffset, float scaleFactor, bool enablePrecheck)
{
    if (!m_pEntity || !m_pEntity->mat || !pointStart || !pointEnd || !pOffset) {
        return false;
    }

    const auto* modelInfo = reinterpret_cast<const CBaseModelInfo*>(CCommon::GetModelInfoByID(m_pEntity->nModelIndex));
    if (!modelInfo || !modelInfo->m_pColModel) {
        return false;
    }

    const float boundingRadius = *reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(modelInfo->m_pColModel) + 0x24);
    if (enablePrecheck)
    {
        static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x318800), "\xA8\x2E\x00\xF0\x00\x79\x43\xF9\xE1\x03\x00\x91", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x2AB024), "\x20\x08\x40\x2D\x01\x0C\x58\x2D\x43\x08\x23\x1E", 12, "__text");
        if (!verified) {
            return false;
        }

        const auto slide = _dyld_get_image_vmaddr_slide(0);
        const auto* camera = *reinterpret_cast<RwCamera* const*>(SA_ADDR(0x8EF6F0) + slide);
        const RwSphere sphere{{pOffset->x, pOffset->y, pOffset->z}, boundingRadius};
        if (!camera || reinterpret_cast<RwFrustumTestResult (*)(const RwCamera*, const RwSphere*)>(SA_ADDR(0x2AB024) + slide)(camera, &sphere) == 0) {
            return false;
        }
    }

    const CVector start = *pointStart - *pOffset;
    const CVector end = *pointEnd - *pOffset;
    const float colLine[8] = {start.x, start.y, start.z, 0.0f, end.x, end.y, end.z, 0.0f};
    float radius = std::max(boundingRadius, 0.5f);
    if (radius < 1.0f)
	{
        radius *= 1.5f;
    }

    const RwSphere colSphere{{}, radius * scaleFactor};
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x8B2A4), "\x02\x0C\x42\x2D\x00\x04\x40\x2D\x45\x38\x20\x1E", 12, "__text");
    return verified && reinterpret_cast<bool (*)(const float*, const RwSphere*)>(SA_ADDR(0x8B2A4) + _dyld_get_image_vmaddr_slide(0))(colLine, &colSphere);
}

bool CEntity::SetModelIndex(unsigned int uiModel)
{
    if (!pthread_main_np() || !m_pEntity || !m_pEntity->vtable) {
        return false;
    }

    int iWaitPreloaded = 0;

    if (!CStreaming::IsModelLoaded(uiModel))
    {
        CStreaming::RequestModel(uiModel);
        CStreaming::LoadAllRequestedModels(false);
        while (!CStreaming::IsModelLoaded(uiModel))
        {
            usleep(1000);
            iWaitPreloaded++;
            if (iWaitPreloaded > 200)
            {
                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(1, OBFUSCATE("Warning: Model %u wouldn't load in time!"), uiModel);
                }

                return false;
            }
        }
    }

    // DeleteRWObject(); // เหมือนจะส่งผล Crash ใน 64Bit เยอะเลย

    m_pEntity->nModelIndex = static_cast<uint16_t>(uiModel);

    // CEntity::SetModelIndex()
    (*reinterpret_cast<void (*const *)(CEntityGta*, unsigned int)>(m_pEntity->vtable + 0x30))(m_pEntity, uiModel);

    return true;
}

uintptr_t CEntity::GetRWObject()
{
    if (m_pEntity)
    {
        return reinterpret_cast<uintptr_t>(m_pEntity->m_pRwObject);
    }

    return 0;
}

void CEntity::DeleteRWObject()
{
    if (!pthread_main_np() || !m_pEntity || !m_pEntity->vtable || !GetRWObject()) {
        return;
    }

    const auto deleteRWObject = *reinterpret_cast<void (*const *)(CEntityGta*)>(m_pEntity->vtable + 0x48);
    deleteRWObject(m_pEntity);
}

float CEntity::GetDistanceFromLocalPlayerPed()
{
    if (!CGame::GetPlayer()) {
        return 10000.0f;
    }

    if (!m_pEntity) {
        return 10000.0f;
    }

    CMatrix matFromPlayer;

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer() && (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsSpectating() /*|| CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsInRCMode()*/))
    {
        CGame::GetCamera()->GetMatrix(&matFromPlayer);
    }
    else
    {
        CGame::GetPlayer()->GetMatrix(&matFromPlayer);
    }

    CMatrix matThis;
    GetMatrix(&matThis);

    const float deltaX = matThis.pos.x - matFromPlayer.pos.x;
    const float deltaY = matThis.pos.y - matFromPlayer.pos.y;
    const float deltaZ = matThis.pos.z - matFromPlayer.pos.z;

    return sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
}

float CEntity::GetDistanceFromPoint(CVector vec)
{
	if (!m_pEntity) {
		return 10000.0f;
	}

	CMatrix matThis;
	GetMatrix(&matThis);

	float tmpX = (matThis.pos.x - vec.x) * (matThis.pos.x - vec.x);
	float tmpY = (matThis.pos.y - vec.y) * (matThis.pos.y - vec.y);
	float tmpZ = (matThis.pos.z - vec.z) * (matThis.pos.z - vec.z);

	return (float)sqrt(tmpX + tmpY + tmpZ);
}

float CEntity::GetDistanceFromPoint(float X, float Y, float Z)
{
	if (!m_pEntity) {
		return 10000.0f;
	}

	CMatrix matThis;
	GetMatrix(&matThis);

	float distanceSquared = pow(matThis.pos.x - X, 2) + pow(matThis.pos.y - Y, 2) + pow(matThis.pos.z - Z, 2);
	return sqrt(distanceSquared);
}

void CEntity::UpdateRwFrame(CEntityGta* entity)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x1431F0), "\x08\x10\x40\xF9\x68\x00\x00\xB4\x00\x05\x40\xF9", 12, "__text");
    if (verified && entity && entity->m_pRwObject && entity->m_pRwObject->parent)
    {
        reinterpret_cast<void (*)(CEntityGta*)>(SA_ADDR(0x1431F0) + _dyld_get_image_vmaddr_slide(0))(entity);
    }
}

void CEntity::UpdateRwMatrixAndFrame()
{
    if (CCommon::IsGameEntityRenderable(m_pEntity) && m_pEntity->m_pRwObject->parent)
    {
        RwFrame* parent = static_cast<RwFrame*>(m_pEntity->m_pRwObject->parent);
        if (m_pEntity->mat)
        {
            m_pEntity->mat->UpdateRwMatrix(&parent->modelling);
        }

        CEntity::UpdateRwFrame(m_pEntity);
    }
}

void CEntity::UpdateMatrix(CMatrix mat)
{
    if (!pthread_main_np() || !m_pEntity || !m_pEntity->mat || !m_pEntity->vtable || CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    (*reinterpret_cast<void (*const *)(CEntityGta*)>(m_pEntity->vtable + 0x20))(m_pEntity); // CPhysical::Remove
    SetMatrix(mat);
    UpdateRwMatrixAndFrame();
    (*reinterpret_cast<void (*const *)(CEntityGta*)>(m_pEntity->vtable + 0x10))(m_pEntity); // CPhysical::Add
}

void CEntity::CleanUpOldReference(CEntityGta* thiz, CEntityGta **ppAddress)
{
    reinterpret_cast<void (*)(CEntityGta*, CEntityGta**)>(SA_ADDR(0x19275C) + _dyld_get_image_vmaddr_slide(0))(thiz, ppAddress);
}

void CEntity::UpdateRpHAnim(CEntityGta* thiz)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x143204), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9\xFD\x43\x00\x91", 12, "__text");
    if (verified)
    {
        reinterpret_cast<void (*)(CEntityGta*)>(SA_ADDR(0x143204) + _dyld_get_image_vmaddr_slide(0))(thiz);
    }
}

void CEntity::ClearStaticWaitingForCollision()
{
    if (m_pEntity)
    {
        m_pEntity->m_nEntityFlags &= ~8u;
    }
}

bool CEntity::IsEntityIgnored()
{
    if (!m_pEntity) {
        return false;
    }

    // pIgnoreEntity
    return ((uintptr_t)m_pEntity == SA_ADDR(0x741E28) + _dyld_get_image_vmaddr_slide(0));
}

bool CEntity::TestSphereCastVsEntity(uintptr_t CColSphere1, uintptr_t CColSphere2, bool bTrue)
{
    if (m_pEntity && !CCommon::IsGameEntityArePlaceable(m_pEntity) && CCommon::ModelInfoGetColSphereRadius(m_pEntity->nModelIndex) > 0.0)
    {
        if (bTrue)
        {
            return CCommon::SphereCastVsEntity(CColSphere1, CColSphere2, m_pEntity, false);
        }
        else
        {
            return CCommon::SphereCastVsEntity(CColSphere1, CColSphere2, m_pEntity, true);
        }
    }

    return 0;
}

bool CEntity::IsRenderable()
{
    if (!m_pEntity) {
        return false;
    }

    return (m_pEntity->vtable != SA_ADDR(0x49BD50) + _dyld_get_image_vmaddr_slide(0) && GetRWObject());
}