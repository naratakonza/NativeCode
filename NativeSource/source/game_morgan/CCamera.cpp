#include "../../Main.h"
#include "CCamera.h"
#include "Scripts/CScripting.h"

#include <mach-o/dyld.h>

float& CCamera::m_fMouseAccelHorzntl = *reinterpret_cast<float*>(SA_ADDR(0x72DCB8) + _dyld_get_image_vmaddr_slide(0));
float& CCamera::m_f3rdPersonCHairMultX = *reinterpret_cast<float*>(SA_ADDR(0x72DCC0) + _dyld_get_image_vmaddr_slide(0));
float& CCamera::m_f3rdPersonCHairMultY = *reinterpret_cast<float*>(SA_ADDR(0x72DCC4) + _dyld_get_image_vmaddr_slide(0));
CCamera& CCamera::TheCamera = *reinterpret_cast<CCamera*>(SA_ADDR(0x72CF08) + _dyld_get_image_vmaddr_slide(0));
bool CCamera::m_bPCCamera = false;

CCamera::CCamera()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x111390), "\xC8\x30\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x111394), "\x08\x21\x3C\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x138050), "\x00\xC0\x25\x91");
    if (verified)
    {
        m_matPos = reinterpret_cast<CMatrix*>(SA_ADDR(0x72CF08 + 0x970) + _dyld_get_image_vmaddr_slide(0)); // TheCamera->m_mCameraMatrix
    }
	
	m_pAttachedTo = nullptr;
}

void CCamera::SetBehindPlayer()
{
	CScripting::ScriptCommand(&lock_camera_position, 0);
  	CScripting::ScriptCommand(&restore_camera_to_user);
	m_pAttachedTo = nullptr;
  	CScripting::ScriptCommand(&set_camera_behind_player);
  	CScripting::ScriptCommand(&restore_camera_jumpcut);
}

void CCamera::Restore()
{
	m_pAttachedTo = nullptr;
	CScripting::ScriptCommand(&restore_camera_jumpcut);
}

void CCamera::SetPosition(float fX, float fY, float fZ, float fRotationX, float fRotationY, float fRotationZ)
{
	CScripting::ScriptCommand(&restore_camera_to_user);
	m_pAttachedTo = nullptr;
	CScripting::ScriptCommand(&set_camera_position, fX, fY, fZ, fRotationX, fRotationY, fRotationZ);
}

void CCamera::Fade(int iInOut)
{
	m_pAttachedTo = nullptr;
	CScripting::ScriptCommand(&fade, 500, iInOut);
}

void CCamera::LookAtPoint(float fX, float fY, float fZ, int iType)
{
	CScripting::ScriptCommand(&restore_camera_to_user);
	m_pAttachedTo = nullptr;
	CScripting::ScriptCommand(&point_camera, fX, fY, fZ, iType);
}

void CCamera::GetMatrix(CMatrix* Matrix)
{
    if (!Matrix || !m_matPos) {
        return;
    }

	Matrix->right.x = m_matPos->right.x;
	Matrix->right.y = m_matPos->right.y;
	Matrix->right.z = m_matPos->right.z;

	Matrix->up.x = m_matPos->up.x;
	Matrix->up.y = m_matPos->up.y;
	Matrix->up.z = m_matPos->up.z;

	Matrix->at.x = m_matPos->at.x;
	Matrix->at.y = m_matPos->at.y;
	Matrix->at.z = m_matPos->at.z;

	Matrix->pos.x = m_matPos->pos.x;
	Matrix->pos.y = m_matPos->pos.y;
	Matrix->pos.z = m_matPos->pos.z;
}

float CCamera::GetDistanceToPoint(CVector *pos)
{
    if (!pos || !m_matPos) {
        return 0.0f;
    }

    const auto& camPos = m_matPos->pos;

    const float deltaX = pos->x - camPos.x;
    const float deltaY = pos->y - camPos.y;
    const float deltaZ = pos->z - camPos.z;

    return sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
}

void CCamera::InterpolateCameraPos(CVector *posFrom, CVector *posTo, int time, uint8_t mode)
{
    if (!posFrom || !posTo) {
        return;
    }

	CScripting::ScriptCommand(&restore_camera_to_user);
	CScripting::ScriptCommand(&lock_camera_position1, 1);
	CScripting::ScriptCommand(&set_camera_pos_time_smooth, posFrom->x, posFrom->y, posFrom->z, posTo->x, posTo->y, posTo->z, time, mode);
}

void CCamera::InterpolateCameraLookAt(CVector *posFrom, CVector *posTo, int time, uint8_t mode)
{
    if (!posFrom || !posTo) {
        return;
    }

	CScripting::ScriptCommand(&lock_camera_position, 1);
	CScripting::ScriptCommand(&point_camera_transverse, posFrom->x, posFrom->y, posFrom->z, posTo->x, posTo->y, posTo->z, time, mode);
}

void CCamera::LerpFOV(float from, float to, float time, bool bSetSmoothEnds)
{
	CScripting::ScriptCommand(&set_camera_zoom_from, from, to, static_cast<int>(time), bSetSmoothEnds);
}

void CCamera::TakeControl(CCamera* thiz, CEntityGta* NewTarget, eCamMode CamMode, eSwitchType CamSwitchStyle, int WhoIsTakingControl)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x13A924), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x13AA2C), "\x75\xCA\x18\x79\x74\xD2\x18\x79", 8, "__text");
    if (!verified) {
        return;
    }

    reinterpret_cast<void (*)(CCamera*, CEntityGta*, int16_t, int16_t, int)>(SA_ADDR(0x13A924) + _dyld_get_image_vmaddr_slide(0))(thiz, NewTarget, static_cast<int16_t>(CamMode), static_cast<int16_t>(CamSwitchStyle), WhoIsTakingControl);
}