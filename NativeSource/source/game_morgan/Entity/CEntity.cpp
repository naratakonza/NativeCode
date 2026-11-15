#include "CEntity.h"
#include "../CCommon.h"
#include "../CStreaming.h"
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

}

void CEntity::Remove()
{

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

void CEntity::CleanUpOldReference(CEntityGta* thiz, CEntityGta **ppAddress)
{
    reinterpret_cast<void (*)(CEntityGta*, CEntityGta**)>(SA_ADDR(0x19275C) + _dyld_get_image_vmaddr_slide(0))(thiz, ppAddress);
}