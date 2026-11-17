#include "../../../../Main.h"
#include "CActor.h"
#include "../../CUtil.h"
#include "../../CGame.h"
#include "../../CCommon.h"
#include "../../CStreaming.h"
#include "../../Scripts/CScripting.h"
#include "../../Animation/CAnimManager.h"

CActor::CActor(uint16_t usModel, VECTOR vecPosition, float fRotation)
{
    CNecessary::Log(OBFUSCATE("CActor Initialise"));

    m_pPed = nullptr;
    m_pEntity = nullptr;
    m_dwGTAId = 0;
	m_bInvulnerable = false;

	if (!CStreaming::IsModelLoaded(usModel))
	{
		CStreaming::RequestModel(usModel);
		CStreaming::LoadAllRequestedModels(false);
		while(!CStreaming::IsModelLoaded(usModel)) usleep(1000);
	}

    uint32_t actorGTAId = 0;
	CScripting::ScriptCommand(&create_actor, 5, usModel, vecPosition.x, vecPosition.y, vecPosition.z - 1.0, &actorGTAId);
	CScripting::ScriptCommand(&set_actor_z_angle, actorGTAId, fRotation);

    m_dwGTAId = actorGTAId;
    m_pPed = (CPedGta*)CUtil::GamePool_Ped_GetAt(m_dwGTAId);
    m_pEntity = (CPhysicalGta*)m_pPed;

	CScripting::ScriptCommand(&set_actor_can_be_decapitated, m_dwGTAId, 0);
	CScripting::ScriptCommand(&set_actor_decision_marker, m_dwGTAId, 65542);
	CScripting::ScriptCommand(&set_char_never_targeted, m_dwGTAId, 1); // 1 ปิดล็อค & 0 เปิดล็อค
}

CActor::~CActor()
{
    CNecessary::Log(OBFUSCATE("CActor Uninitialise"));

    if (!CCommon::IsValidGamePed(m_pPed) || !CUtil::GamePool_Ped_GetAt(m_dwGTAId) || CCommon::IsGameEntityArePlaceable(m_pEntity))
	{
		m_pPed = nullptr;
		m_pEntity = nullptr;
		m_dwGTAId = 0;

		return;
	}

	// ลบ
    reinterpret_cast<void(*)(CPedGta*)>(*reinterpret_cast<void**>(m_pPed->vtable + 0x8))(m_pPed);

	m_pPed = nullptr;
	m_pEntity = nullptr;
	m_dwGTAId = 0;

	return;
}

void CActor::SetPosition(VECTOR vecPosition)
{
	if (!CCommon::IsValidGamePed(m_pPed)) {
		return;
	}

	TeleportTo(vecPosition.x, vecPosition.y, vecPosition.z);
}

void CActor::ApplyAnimation(char *szAnimName, char *szAnimFile, float fDelta, int bLoop, int bLockX, int bLockY, int bFreeze, int uiTime)
{
	if (!CCommon::IsValidGamePed(m_pPed) || !CUtil::GamePool_Ped_GetAt(m_dwGTAId)) {
		return;
	}

	if (!szAnimName || !szAnimFile) {
		return;
	}

	if (!strcasecmp(szAnimFile, OBFUSCATE("SEX"))) {
		return;
	}

	if (!CAnimManager::GetAnimationBlock(szAnimFile)) {
		CExceptions::Print(OBFUSCATE("opcode"), OBFUSCATE("0x4EE"));
		return;
	}

	int iWaitAnimLoad = 0;

	if (!CGame::IsAnimationLoaded(szAnimFile))
	{
		CGame::RequestAnimation(szAnimFile);
		while (!CGame::IsAnimationLoaded(szAnimFile))
		{
			usleep(1000);
			iWaitAnimLoad++;
			if (iWaitAnimLoad > 15) 
			{
				return;
			}
		}
	}

	CScripting::ScriptCommand(&apply_animation, m_dwGTAId, szAnimName, szAnimFile, fDelta, bLoop, bLockX, bLockY, bFreeze, uiTime);
}

void CActor::ClearAnimations()
{
	if (!CCommon::IsValidGamePed(m_pPed)) {
		return;
	}

	CPedIntelligence::FlushImmediately(m_pPed->pPedIntelligence, true);
}

void CActor::SetAngle(float fAngle)
{
	if (!CCommon::IsValidGamePed(m_pPed)) {
		return;
	}

	m_pPed->m_fDesiredHeading = CUtil::DegToRad(fAngle);
}

void CActor::SetHealth(float fHealth)
{
	if (!CCommon::IsValidGamePed(m_pPed)) {
		return;
	}

	m_pPed->fHealth = fHealth;
	if (m_pPed->fHealth <= 0.0f)
	{
		CScripting::ScriptCommand(&kill_actor, m_dwGTAId);
	}
}

void CActor::SetInvulnerable(bool bEnable)
{
	if (!CCommon::IsValidGamePed(m_pPed)) {
		return;
	}

	m_bInvulnerable = bEnable;
	
	if (bEnable)
	{
		CScripting::ScriptCommand(&set_actor_immunities, m_dwGTAId, 1, 1, 1, 1, 1);
	}
	else
	{
		CScripting::ScriptCommand(&set_actor_immunities, m_dwGTAId, 0, 0, 0, 1, 0);
	}
}