#include "../../RenderWare/rphanim.h"
#include "../../Animation/CAnimBlendClumpData.h"
#include "../../Animation/BoneNode_c.h"
#include "../../Enums/eBoneTag.h"

#include "CPlayerPed.h"
#include "CPed.h"
#include "CPlayerPedSlot.h"
#include "CPedPointerRecorder.h"
#include "../../CGame.h"
#include "../../CCameraController.h"
#include "../../CUtil.h"
#include "../../CWorld.h"
#include "../../CPedIntelligence.h"
#include "../../CCommon.h"
#include "../../CStreaming.h"
#include "../../CWeapon.h"
#include "../../Base/CPad.h"
#include "../../Animation/CAnimManager.h"
#include "../../Animation/CAnimation.h"
#include "../../Enums/eSpecialObject.h"
#include "../../Enums/eSpecialVehicle.h"
#include "../../Enums/eWeaponSampid.h"
#include "../../Scripts/CScripting.h"
#include "../../Tasks/TaskTypes/CTaskSimpleJetPack.h"
#include "../../Multiplayer/CMultiPlayer.h"
#include "../../../utils/Exceptions/CExceptions.h"
#include "../../../utils/Necessary/CNecessary.h"

#include <mach-o/dyld.h>
#include <unistd.h>
#include <strings.h>

char DanceStyleLibs[4][16] = { "WOP", "GFUNK", "RUNNINGMAN", "STRIP" };
char DanceIdleLoops[4][16] = { "DANCE_LOOP", "DANCE_LOOP", "DANCE_LOOP", "STR_Loop_B" };
char szDanceAnimNamesFemale[16][16] =
{
    "DANCE_G1","DANCE_G2","DANCE_G3","DANCE_G4",
    "DANCE_G5","DANCE_G6","DANCE_G7","DANCE_G8",
    "DANCE_G9","DANCE_G10","DANCE_G11","DANCE_G12",
    "DANCE_G13","DANCE_G14","DANCE_G15","DANCE_G16"
};

char szDanceAnimNamesMale[16][16] =
{
    "DANCE_B1","DANCE_B2","DANCE_B3","DANCE_B4",
    "DANCE_B5","DANCE_B6","DANCE_B7","DANCE_B8",
    "DANCE_B9","DANCE_B10","DANCE_B11","DANCE_B12",
    "DANCE_B13","DANCE_B14","DANCE_B15","DANCE_B16"
};

char szStripAnims[16][16] =
{
    "strip_A","strip_B","strip_C","strip_D",
    "strip_E","strip_F","strip_G","STR_A2B",
    "STR_B2A","STR_B2C","STR_C1","STR_C2",
    "STR_C2B","STR_A2B","STR_B2C","STR_C2"
};

bool CPlayerPed::m_bRebuildPlayer = true;
uint32_t m_dwJumpTickCounter = 0;
uint32_t m_dwMeleeAttackTickCounter = 0;
uint32_t m_dwDiveTickCounter = 0;
uint32_t m_dwAbortClimbTickCounter = 0;

CPlayerPed::CPlayerPed()
{
    //CNecessary::Log(OBFUSCATE("CPlayerPed (Line: %d)"), __LINE__);

    m_dwGTAId = 1;
    m_pPed = nullptr;
    m_pEntity = nullptr;

    m_pPed = CUtil::GamePool_FindPlayerPed();
	m_pEntity = reinterpret_cast<CPhysicalGta*>(CUtil::GamePool_FindPlayerPed());

	CPlayerPed::m_bytePlayerNumber = 0;
	CPlayerPed::m_dwArrow = 0;
	CPlayerPed::m_bLockControl = false;
	CPlayerPed::m_bPissingState = false;
	CPlayerPed::m_iCellPhoneEnabled = 0;
	CPlayerPed::m_bCuffedState = false;
	CPlayerPed::m_bCarryState = false;
	CPlayerPed::m_bDanceState = false;
	CPlayerPed::m_iDanceStyle = -1;
	CPlayerPed::m_iAlpha = 255;
	CPlayerPed::m_dwParachuteObject = 0;

	CPedPointerRecorder::Record(CPlayerPed::m_bytePlayerNumber, reinterpret_cast<uintptr_t>(m_pPed));
	CPlayerPedSlot::Set(CPlayerPed::m_bytePlayerNumber, 1);
    
	CScripting::ScriptCommand(&set_actor_weapon_droppable, m_dwGTAId, 1);
	CScripting::ScriptCommand(&set_actor_can_be_decapitated, m_dwGTAId, 0);

	CPlayerPed::m_stuffData.dwDrunkLevel = 0;
	CPlayerPed::m_stuffData.dwObject = 0;
	CPlayerPed::m_stuffData.type = eStuffType::STUFF_TYPE_NONE;
	CPlayerPed::m_stuffData.dwLastUpdateTick = 0;
	CPlayerPed::m_bJumpControl = false;
	CPlayerPed::m_bCrouchControl = false;
	CPlayerPed::m_bSprintControl = false;

    // เดียวมาทำต่อภายหลัง
	// CPlayerPed::DeleteAllAccessories();

    CPlayerPed::m_bHaveBulletData = false;
	memset(&m_bulletData, 0, sizeof(BULLET_DATA));
}

CPlayerPed::CPlayerPed(uint8_t bytePlayerNumber, int iSkin, float fX, float fY, float fZ, float fRotation)
{
    // CNecessary::Log(OBFUSCATE("CPlayerPed (Line: %d): bytePlayerNumber: %d, iSkin: %d, XYZ: %.2f %.2f %.2f, fRotation: %.2f"), __LINE__, bytePlayerNumber, iSkin, fX, fY, fZ, fRotation);    

    uint32_t dwPlayerActorID = 0;
    int iPlayerNum = bytePlayerNumber;
    m_pPed = nullptr;
	m_pEntity = nullptr;
    
    m_dwGTAId = 0;
	CPlayerPed::m_dwArrow = 0;
    CPlayerPed::m_bLockControl = false;
	CPlayerPed::m_bPissingState = false;
	CPlayerPed::m_iCellPhoneEnabled = 0;
	CPlayerPed::m_bCuffedState = false;
	CPlayerPed::m_bCarryState = false;
	CPlayerPed::m_bDanceState = false;
	CPlayerPed::m_iDanceStyle = -1;
	CPlayerPed::m_iAlpha = 255;
	CPlayerPed::m_dwParachuteObject = 0;

	CScripting::ScriptCommand(&create_player, &iPlayerNum, fX, fY, fZ, &dwPlayerActorID);
	CScripting::ScriptCommand(&create_actor_from_player, &iPlayerNum, &dwPlayerActorID);
    
    m_dwGTAId = dwPlayerActorID;
    m_pPed = CUtil::GamePool_Ped_GetAt(m_dwGTAId);
    m_pEntity = reinterpret_cast<CPhysicalGta*>(CUtil::GamePool_Ped_GetAt(m_dwGTAId));
    CPlayerPed::m_bytePlayerNumber = bytePlayerNumber;

	CPedPointerRecorder::Record(CPlayerPed::m_bytePlayerNumber, reinterpret_cast<uintptr_t>(m_pPed));

	CScripting::ScriptCommand(&set_actor_weapon_droppable, m_dwGTAId, 1);
	CScripting::ScriptCommand(&set_actor_immunities, m_dwGTAId, 0, 0, 1, 0, 0);
	CScripting::ScriptCommand(&set_actor_can_be_decapitated, m_dwGTAId, 0);

	if (CMultiPlayer::GetActiveNetwork()) 
    {
       CPlayerPed::SetMoney(CMultiPlayer::GetActiveNetwork()->m_iDeathDropMoney);
    }

	CPlayerPed::SetModelIndex(iSkin);

    if (m_pPed && CUtil::GamePool_Ped_GetAt(m_dwGTAId))
    {
		CPlayerPedSlot::Set(CPlayerPed::m_bytePlayerNumber, 1);

    	CPlayerPed::ForceTargetRotation(fRotation);
    }

	CMatrix mat;
    GetMatrix(&mat);
    mat.pos.x = fX;
    mat.pos.y = fY;
    mat.pos.z = fZ + 0.15f;
    SetMatrix(mat);

	CPlayerPed::m_stuffData.dwDrunkLevel = 0;
	CPlayerPed::m_stuffData.dwObject = 0;
	CPlayerPed::m_stuffData.type = eStuffType::STUFF_TYPE_NONE;
	CPlayerPed::m_stuffData.dwLastUpdateTick = 0;
	CPlayerPed::m_bJumpControl = false;
	CPlayerPed::m_bCrouchControl = false;
	CPlayerPed::m_bSprintControl = false;

    // เดียวมาทำต่อภายหลัง
	// CPlayerPed::DeleteAllAccessories();

	CPlayerPed::m_bHaveBulletData = false;
	memset(&m_bulletData, 0, sizeof(BULLET_DATA));
}

CPlayerPed::~CPlayerPed()
{
    CNecessary::Log(OBFUSCATE("CPlayerPed Uninitialise"));

	CPedPointerRecorder::Record(CPlayerPed::m_bytePlayerNumber, 0);
	CPlayerPedSlot::Set(CPlayerPed::m_bytePlayerNumber, 0);

	CPlayerPed::m_bHaveBulletData = false;
	memset(&m_bulletData, 0, sizeof(BULLET_DATA));

    if (!m_pPed || !CCommon::IsValidGamePed(m_pPed) || !CUtil::GamePool_Ped_GetAt(m_dwGTAId) || CCommon::IsGameEntityArePlaceable(m_pEntity))
    {
        m_pPed = nullptr;
        m_pEntity = nullptr;
        m_dwGTAId = 0;
        
        return;
    }

	CPlayerPed::DestroyParachute();

    // เดียวมาทำต่อภายหลัง
	// if (CPlayerPed::HasAccessory())
	// {
	// 	CPlayerPed::DeleteAllAccessories(true);
	// }

	if (CPlayerPed::GetStuff())
	{
		CPlayerPed::DropStuff();
	}

    if (CPlayerPed::IsInVehicle())
	{
		CPlayerPed::RemoveFromVehicleAndPutAt(100.0f, 100.0f, 20.0f);
	}

	// แก้ไขปัญหาการลบตัวละครผู้เล่น
	uintptr_t dwPedPtr = reinterpret_cast<uintptr_t>(m_pPed);
	*reinterpret_cast<uint32_t*>(*reinterpret_cast<uintptr_t*>(dwPedPtr + 1344) + 76) = 0; // m_pPed->m_pPlayerData->m_nPlayerGroup

	// CPlayerPed::Destructor
    reinterpret_cast<void(*)(CPedGta*)>(*reinterpret_cast<void**>(GetPed()->vtable + 0x8))(m_pPed);

    m_pPed = nullptr;
    m_pEntity = nullptr;
    m_dwGTAId = 0;
}

bool CPlayerPed::VerifyInstance()
{
    if (m_pPed && CCommon::IsValidGamePed(m_pPed) && CUtil::GamePool_Ped_GetAt(static_cast<int>(m_dwGTAId)) == m_pPed)
    {
        return true;
    }

    return false;
}

void CPlayerPed::ClumpUpdateAnimations(float step, int flag)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    uintptr_t pRwObj = GetRWObject();
    if (pRwObj)
    {
        CUtil::RpAnimBlendClumpUpdateAnimations(pRwObj, step, flag);
    }
}

void CPlayerPed::SetModelIndex(unsigned int uiModel)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (!CCommon::IsPedModel(uiModel)) {
        uiModel = 0;
    }

    if (!CStreaming::IsObjectInCdImage(static_cast<int>(uiModel), OBFUSCATE("0x9C7"))) {
        CNecessary::Log(OBFUSCATE("SetModelIndex: ModelID %u is not present in game files (CdImage)"), uiModel);
        uiModel = 0;
    }

    if (m_pPed)
    {
        CPlayerPed::m_bRebuildPlayer = false; // ใช้แบบ เพื่อป้องกันเกมค้าง

        CPlayerPed::DestroyFollowPedTask(); // คิดว่าทำมาตรงสมบูรณ์แล้ว
        CEntity::SetModelIndex(uiModel);

        static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x23B2E4), "\x60\x02\x0D\x91\xE1\x03\x13\xAA\x77\x75\xF8\x97", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x588DC), "\x9F\xEE\x0B\xB8\x81\xA2\x14\xF8\x9F\xA2\x00\xF8\x08\x59\xB8\x52", 16, "__text");
        if (verified)
        {
            reinterpret_cast<void (*)(void*, CPedGta*)>(SA_ADDR(0x588C8) + _dyld_get_image_vmaddr_slide(0))(GetPed()->m_PedSpeechAudioEntity, GetPed());
        }
    }
}

void CPlayerPed::DestroyFollowPedTask()
{
    if (!CPlayerPed::VerifyInstance() || !GetPed()->pPedIntelligence) {
        return;
    }

    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x3B92B4), "\x1F\x04\x05\xA9") && CMemory::HasMappedBytes(SA_ADDR(0x360F6C), "\x48\x0A\x00\xF0\x08\xE1\x22\x91\x08\x00\x00\xF9\x00\x08\x40\xF9", 16, "__text");
    if (!verified) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    CTask*& ikManager = GetPed()->pPedIntelligence->m_taskManager.m_aSecondaryTasks[eSecondaryTask::TASK_SECONDARY_IK];

    if (ikManager && ikManager->vtable == SA_ADDR(0x4AB8B8) + slide)
    {
        reinterpret_cast<void (*)(CTask*)>(SA_ADDR(0x360F5C) + slide)(ikManager);
        ikManager = nullptr;
    }
}

void CPlayerPed::SetMoney(int iAmount)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    CScripting::ScriptCommand(&set_actor_money, m_dwGTAId, 0);
    CScripting::ScriptCommand(&set_actor_money, m_dwGTAId, iAmount);
}

void CPlayerPed::SetInterior(uint8_t byteID, bool refresh)
{
    if (!CPlayerPed::VerifyInstance()) {
	    return;
    }

    if (GetPed() && CPlayerPed::m_bytePlayerNumber != 0) 
    {
		CScripting::ScriptCommand(&link_actor_to_interior, m_dwGTAId, byteID);
	}
	else
	{
		CScripting::ScriptCommand(&select_interior, byteID);
		CScripting::ScriptCommand(&link_actor_to_interior, m_dwGTAId, byteID);

		if (refresh)
		{
			CPlayerPed::RefreshStreamingByPos();
		}
	}
}

void CPlayerPed::RefreshStreamingByPos() 
{
    CMatrix mat;
    GetMatrix(&mat);
    CGame::RefreshStreamingAt(mat.pos);
}

bool CPlayerPed::IsInVehicle()
{
    return CPlayerPed::VerifyInstance() && GetPed()->bInVehicle;
}

bool CPlayerPed::IsInWater()
{
    if (!CPlayerPed::VerifyInstance()) {
        return false;
    }

    return CScripting::ScriptCommand(&is_actor_in_the_water, m_dwGTAId);
}

bool CPlayerPed::IsAPassenger()
{
	if (CPlayerPed::IsInVehicle() && GetPed()->pVehicle)
	{
		CVehicleGta *pVehicle = GetPed()->pVehicle;

        if (pVehicle->pDriver != GetPed() || pVehicle->nModelIndex == eSpecialVehicle::TRAIN_PASSENGER || pVehicle->nModelIndex == eSpecialVehicle::TRAIN_FREIGHT) {
            return true;
        }
	}

	return false;
}

void CPlayerPed::ProcessAnimGroups(bool bUseCJWalk)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x26FCB8), "\x27\x9E\x08\x14") && CMemory::HasMappedBytes(SA_ADDR(0x497554), "\xC9\x33\x00\x90\x29\xE1\x73\x39\x49\x00\x00\x35\x68\x32\x06\xB9\xD6\x61\xF7\x17", 20, "__mg_gameproc");
    if (!verified) {
        return;
    }

    *reinterpret_cast<std::uint8_t*>(SA_ADDR(0xB0FCF8) + _dyld_get_image_vmaddr_slide(0)) = static_cast<std::uint8_t>(!bUseCJWalk);
}

void CPlayerPed::SetFightingStyle(int iStyle)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	GetPed()->m_nExtraMeleeCombo = iStyle;
	GetPed()->m_nExtraMeleeComboFlags = 6;
}

int CPlayerPed::GetFightingStyle()
{
	if (!CPlayerPed::VerifyInstance()) {
		return 4;
	}

	return GetPed()->m_nExtraMeleeCombo;
}

void CPlayerPed::SetImmunities(int BP, int FP, int EP, int CP, int MP)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	if (CUtil::GamePool_Ped_GetAt(m_dwGTAId))
	{
		CScripting::ScriptCommand(&set_actor_immunities, m_dwGTAId, BP, FP, EP, CP, MP);
	}
}

void CPlayerPed::ShowMarker(uint32_t iMarkerColorID)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	if (m_dwArrow)
	{
		CPlayerPed::HideMarker();
	}

	CScripting::ScriptCommand(&create_arrow_above_actor, m_dwGTAId, &m_dwArrow);
	CScripting::ScriptCommand(&set_marker_color, m_dwArrow, iMarkerColorID);
	CScripting::ScriptCommand(&show_on_radar2, m_dwArrow, 2);
}

void CPlayerPed::HideMarker()
{
	if (m_dwArrow)
	{
		CScripting::ScriptCommand(&disable_marker, m_dwArrow);
	}

	m_dwArrow = 0;
}

void CPlayerPed::RemoveFromVehicleAndPutAt(float fX, float fY, float fZ)
{
    if (CPlayerPed::IsInVehicle())
    {
        CScripting::ScriptCommand(&remove_actor_from_car_and_put_at, static_cast<int>(m_dwGTAId), fX, fY, fZ);
    }
}

void CPlayerPed::SetStateFlags(uint32_t dwState)
{
    if (!CPlayerPed::VerifyInstance())
    {
        return;
    }

    GetPed()->m_nPedFlags = dwState;
}

uint32_t CPlayerPed::GetStateFlags()
{
    if (!CPlayerPed::VerifyInstance())
    {
        return 0;
    }

    return GetPed()->m_nPedFlags;
}

bool CPlayerPed::IsOnGround()
{
    if (!CPlayerPed::VerifyInstance())
    {
        return false;
    }

    return (CPlayerPed::GetStateFlags() & 3) != 0;
}

void CPlayerPed::SetHealth(float hp)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

    GetPed()->fHealth = hp;
}

float CPlayerPed::GetHealth()
{
	if (!GetPed()) {
		return 0.0f;
	}

    return GetPed()->fHealth;
}

void CPlayerPed::SetArmour(float fArmour)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	GetPed()->fArmour = fArmour;
}

float CPlayerPed::GetArmour()
{
	if (!GetPed()) {
		return 0.0f;
	}

	return GetPed()->fArmour;
}

bool CPlayerPed::IsDead()
{
	if (!CPlayerPed::VerifyInstance()) {
		return false;
	}

	if (GetPed()->fHealth > 0.0f) {
		return false;
	}

	return true;
}

void CPlayerPed::SetActionTrigger(uint8_t action)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	GetPed()->m_nPedState = static_cast<ePedState>(action);
}

uint8_t CPlayerPed::GetActionTrigger()
{
	return GetPed()->m_nPedState;
}

void CPlayerPed::RestartIfWastedAt(CVector *vecRestart, float fRotation)
{
	CScripting::ScriptCommand(&restart_if_wasted_at, vecRestart->x, vecRestart->y, vecRestart->z, fRotation, 0);
}

void CPlayerPed::SetInitialState()
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x26CD04), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9\xF4\x4F\x02\xA9\xFD\x7B\x03\xA9\xFD\xC3\x00\x91\xF4\x03\x01\xAA\xF3\x03\x00\xAA", 28, "__text");
	if (!verified) {
		return;
	}

	reinterpret_cast<void (*)(CPedGta*, bool)>(SA_ADDR(0x26CD04) + _dyld_get_image_vmaddr_slide(0))(GetPed(), false);
}

void CPlayerPed::ResetDamageEntity()
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

    GetPed()->pLastDamageEntity = nullptr;
	GetPed()->LastDamagedWeaponType = -1;
}

void CPlayerPed::LockControl(bool bLock)
{
	CPlayerPed::m_bLockControl = bLock;
}

void CPlayerPed::ToggleControllable(bool bToggle)
{
	CNecessary::Log(OBFUSCATE("CPlayerPed ToggleControllable %d"), bToggle);

	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	CMatrix mat;

	if (!bToggle)
	{
		CScripting::ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 0);
		CScripting::ScriptCommand(&lock_actor, m_dwGTAId, 1);
	}
	else
	{
		if (!CPlayerPed::m_bLockControl)
		{
			CScripting::ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 1);
			CScripting::ScriptCommand(&lock_actor, m_dwGTAId, 0);

			if (!CPlayerPed::IsInVehicle()) 
            {
                GetMatrix(&mat);
                TeleportTo(mat.pos.x, mat.pos.y, mat.pos.z);
		    }
		}
	}
}

void CPlayerPed::GiveWeapon(int iWeaponID, int iAmmo)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	if (!CUtil::GamePool_FindPlayerPed()) {
		return;
	}

	int iModelID = 0;
	iModelID = CWeapon::GameGetWeaponModelIDFromWeaponID(iWeaponID);

	if (iModelID == -1) {
		return;
	}

	if (!CStreaming::IsModelLoaded(iModelID))
	{
		CStreaming::RequestModel(iModelID);
		CStreaming::LoadAllRequestedModels();
		while (!CStreaming::IsModelLoaded(iModelID)) usleep(1000);
	}
	
	CWorld::PlayerInFocus = CPlayerPed::m_bytePlayerNumber;

	CCameraAim::GameStoreLocalPlayerSkills();
    CCameraAim::GameSetRemotePlayerSkills(CPlayerPed::m_bytePlayerNumber);
	
	CPed::GiveWeapon(GetPed(), iWeaponID, iAmmo, true);

	CCameraAim::GameSetLocalPlayerSkills();

	CPlayerPed::SetArmedWeapon(iWeaponID);

	CWorld::PlayerInFocus = 0;
}

void CPlayerPed::SetArmedWeapon(int iWeaponID, bool unk)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	if (!CUtil::GamePool_FindPlayerPed()) {
		return;
	}

	CWorld::PlayerInFocus = CPlayerPed::m_bytePlayerNumber;
	
	CCameraAim::GameStoreLocalPlayerSkills();
    CCameraAim::GameSetRemotePlayerSkills(CPlayerPed::m_bytePlayerNumber);

	if (unk)
	{
		CPed::SetCurrentWeapon(GetPed(), iWeaponID);
	}
	else
	{
		CScripting::ScriptCommand(&set_actor_armed_weapon, m_dwGTAId, iWeaponID);
	}

	CCameraAim::GameSetLocalPlayerSkills();
	CWorld::PlayerInFocus = 0;
}

void CPlayerPed::SetWeaponSkill(uint32_t iWeaponType, uint16_t byteSkill)
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	if (CPlayerPed::m_bytePlayerNumber == 0)
	{
		CCameraAim::GameUpdateLocalPlayerSkill(iWeaponType, byteSkill);
	}
	else
	{
		CCameraAim::GameStoreRemotePlayerSkills(CPlayerPed::m_bytePlayerNumber, iWeaponType, byteSkill);
	}
}

void CPlayerPed::SetAmmo(uint8_t bWeaponID, uint16_t wAmmo)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    WEAPON_SLOT_TYPE* WeaponSlot = CPlayerPed::FindWeaponSlot(bWeaponID);
    if (WeaponSlot)
    {
        WeaponSlot->dwAmmo = static_cast<uint32_t>(wAmmo);
    }
}

WEAPON_SLOT_TYPE* CPlayerPed::FindWeaponSlot(uint32_t dwWeapon)
{
    if (!CPlayerPed::VerifyInstance()) {
        return nullptr;
    }

    for (uint8_t i = 0; i < 13; i++)
    {
        if (GetPed()->WeaponSlots[i].dwType == dwWeapon)
        {
            return &GetPed()->WeaponSlots[i];
        }
    }

    return nullptr;
}

WEAPON_SLOT_TYPE* CPlayerPed::GetCurrentWeaponSlot()
{
	if (GetPed())
	{
		return &GetPed()->WeaponSlots[GetPed()->byteCurWeaponSlot];
	}

	return (WEAPON_SLOT_TYPE*)NULL;
}

uint8_t CPlayerPed::GetCurrentWeapon()
{
	if (!CPlayerPed::VerifyInstance()) {
		return 0;
	}

	return GetPed()->WeaponSlots[GetPed()->byteCurWeaponSlot].dwType;
}

bool CPlayerPed::CurrentWeaponHasAmmo()
{
    if (!CPlayerPed::VerifyInstance()) {
        return false;
    }

    WEAPON_SLOT_TYPE* pSlot = CPlayerPed::GetCurrentWeaponSlot();
    if (!pSlot) {
        return false;
    }

    uint32_t weaponId = pSlot->dwType;
    if (weaponId <= eWeaponSampId::WEAPON_CANE || weaponId == eWeaponSampId::WEAPON_PARACHUTE) {
        return false;
    }

    return (pSlot->dwAmmo > 0);
}

void CPlayerPed::GetAllWeapons(uint8_t* pOutWeapons)
{
    if (!CPlayerPed::VerifyInstance() || !pOutWeapons) {
		return;
	}

    for (int i = 0; i < 13; ++i)
    {
        pOutWeapons[i] = static_cast<uint8_t>(GetPed()->WeaponSlots[i].dwType);
    }
}

void CPlayerPed::ClearAllWeapons()
{
	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	CWorld::PlayerInFocus = CPlayerPed::m_bytePlayerNumber;
	CPed::ClearWeapons(GetPed());
	CWorld::PlayerInFocus = 0;
}

void CPlayerPed::ForceTargetRotation(float fRotation)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    GetPed()->m_fCurrentHeading = CUtil::DegToRad(fRotation);
    GetPed()->m_fDesiredHeading = CUtil::DegToRad(fRotation);

    CScripting::ScriptCommand(&set_actor_z_angle, m_dwGTAId, fRotation);
}

void CPlayerPed::SetRotation(float fRotation)
{
	if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    GetPed()->m_fCurrentHeading = CUtil::DegToRad(fRotation);
    GetPed()->m_fDesiredHeading = CUtil::DegToRad(fRotation);
}

uint8_t CPlayerPed::GetExtendedKeys()
{
	uint8_t result = 0;
	if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_YES])
	{
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_YES] = false;
		result = 1;
	}

	if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_NO])
	{
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = false;
		result = 2;
	}

	if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK])
	{
		CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK] = false;
		result = 3;
	}

	return result;
}

void CPlayerPed::ShuffleKeyLogic(int key)
{
	if (key == ePadKeys::KEY_FIRE)
	{
		if (CNecessary::GetTickCount() < m_dwDiveTickCounter)
		{
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = true;
		}
	}

	if (key == ePadKeys::KEY_JUMP)
	{
		if (CNecessary::GetTickCount() < m_dwJumpTickCounter)
		{
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = true;
		}
	}

	if (key == ePadKeys::KEY_HANDBRAKE)
	{
		// KEY_HANDBRAKE ของฟังชั่นนี้ ทำให้ส่งผลภาพไม่ตรงในระบบ หมัด F และบล็อค มันถูก Reset
		if (CPad::m_iSaveBlockPunch) {
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = true;
			return;
		}

		if (CNecessary::GetTickCount() < m_dwMeleeAttackTickCounter) {
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = true;
			return;
		}

		if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] && CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK]) {
			return;
		}

		if (CPad::m_iSaveTargetingActive || CCameraController::IsCameraAllAiming(false, false))
		{
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = true;
		}
		else
		{
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = false;
		}
	}

	if (key == ePadKeys::KEY_SECONDARY_ATTACK)
	{
		if (CNecessary::GetTickCount() < m_dwMeleeAttackTickCounter || CNecessary::GetTickCount() < m_dwAbortClimbTickCounter)
		{
			CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true;
		}
	}
}

void CPlayerPed::SetKeys(uint16_t wKeys, uint16_t lrAnalog, uint16_t udAnalog)
{
    if (!CPlayerPedSlot::IsActive(CPlayerPed::m_bytePlayerNumber)) {
        return;
    }

    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    PAD_KEYS *pad = &CPad::RemotePlayerKeys[CPlayerPed::m_bytePlayerNumber];

    pad->wKeyLR = lrAnalog;
    pad->wKeyUD = udAnalog;

    for (int i = 0; i < ePadKeys::SIZE; i++)
	{
        pad->bKeys[i] = (wKeys & 1);
        if (i == ePadKeys::KEY_JUMP)
		{
			if (!pad->bKeys[ePadKeys::KEY_JUMP])
			{
				pad->bIgnoreJump = false;
			}
		}
		
        wKeys >>= 1;
    }
}

uint16_t CPlayerPed::GetKeys(uint16_t* lrAnalog, uint16_t* udAnalog, bool clear)
{
	*lrAnalog = CPad::LocalPlayerKeys.wKeyLR;
	*udAnalog = CPad::LocalPlayerKeys.wKeyUD;

	uint16_t wRet = 0;
	uint16_t keyMask = 1;

	for (int i = 0; i < ePadKeys::SIZE; i++)
	{
		CPlayerPed::ShuffleKeyLogic(i);

	    if (CPad::LocalPlayerKeys.bKeys[i])
		{
	        wRet |= keyMask;
	    }

	    keyMask <<= 1;
	}

	if (clear)
	{
		memset(CPad::LocalPlayerKeys.bKeys, 0, ePadKeys::SIZE);
		CPad::LocalPlayerKeys.wKeyLR = 0;
		CPad::LocalPlayerKeys.wKeyUD = 0;
	}

	return wRet;
}

void CPlayerPed::StartJetpack()
{
    if (!CPlayerPed::VerifyInstance())
    {
        return;
    }

    CWorld::PlayerInFocus = CPlayerPed::m_bytePlayerNumber;
    TeleportTo(GetPed()->mat->pos.x, GetPed()->mat->pos.y, GetPed()->mat->pos.z);
    CScripting::ScriptCommand(&actor_task_jetpack, m_dwGTAId);
    CWorld::PlayerInFocus = 0;
}

void CPlayerPed::StopJetpack()
{
    if (!CPlayerPed::VerifyInstance() || !GetPed()->pPedIntelligence)
    {
        return;
    }

    CTask* taskMgr = GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY];
    if (!taskMgr)
    {
        return;
    }

    uintptr_t taskVtbl = *reinterpret_cast<uintptr_t*>(taskMgr);
    if (taskVtbl == SA_ADDR(0x4B17B8) + _dyld_get_image_vmaddr_slide(0))
    {
        CTaskSimpleJetPack::Destructor(reinterpret_cast<uintptr_t>(taskMgr));
        GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY] = nullptr;
    }
}

bool CPlayerPed::IsInJetpack()
{
    if (!CPlayerPed::VerifyInstance() || !GetPed()->pPedIntelligence)
    {
        return false;
    }

    CTask* task = GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY];
    if (!task)
    {
        return false;
    }

    return task->vtable == SA_ADDR(0x4B17B8) + _dyld_get_image_vmaddr_slide(0);
}

uint8_t CPlayerPed::IsEnteringVehicle()
{
    if (!CPlayerPed::VerifyInstance()) {
        return 0;
    }

    if (GetPed() && GetPed()->pPedIntelligence && GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY])
    {
        int iType = CCommon::GetTaskTypeFromTask(reinterpret_cast<uintptr_t*>(GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY]));

        if (iType == 700 || iType == 712)
        {
            return 2;
        }
        
        if (iType == 701 || iType == 713)
        {
            return 1;
        }
    }

    return 0;
}

bool CPlayerPed::IsExitingVehicle()
{
    if (!CPlayerPed::VerifyInstance())
    {
        return false;
    }

    if (GetPed() && GetPed()->pPedIntelligence && GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY])
    {
        return CCommon::GetTaskTypeFromTask(reinterpret_cast<uintptr_t*>(GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY])) == 704;
    }

    return false;
}

void CPlayerPed::GetBonePosition(int iBoneID, CVector* vecOut)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    CPed::GetBonePosition(GetPed(), vecOut, iBoneID, 0);
}

CEntityGta* CPlayerPed::GetTargetEntity()
{
    if (!CPlayerPed::VerifyInstance()) {
        return nullptr;
    }

    if (CCommon::IsGameEntityArePlaceable(m_pEntity) || !m_pEntity->m_pRwObject) {
        return nullptr;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x26F460), "\xE8\x03\x00\xAA\x00\xC4\x44\xF9", 8, "__text");
    if (!verified) {
        return nullptr;
    }

    // CPlayerPed::m_p3rdPersonMouseTarget อยู่ถัดจาก CPed บน iOS
    return *reinterpret_cast<CEntityGta**>(reinterpret_cast<uintptr_t>(GetPed()) + 0x988);
}

void CPlayerPed::ResetPlayerTimeLastEaten()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x18E6E4), "\x1F\x01\x03\x79");
    if (!verified || !CWorld::Players) {
        return;
    }

    *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(CWorld::Players) + 0x180) = 0; // CWorld::Players->TimeLastEaten
}

void CPlayerPed::SetDead()
{
    if (m_dwGTAId && CCommon::IsValidGamePed(m_pPed))
    {
        if (CPlayerPed::IsInJetpack())
        {
            CPlayerPed::StopJetpack();
        }

        try
        {
            CPlayerPed::ExtinguishFire();
        }
        catch (...)
        {

        }

        CMatrix mat;
        GetMatrix(&mat);
        TeleportTo(mat.pos.x, mat.pos.y, mat.pos.z);

        CPlayerPed::SetHealth(0.0f);
        CWorld::PlayerInFocus = CPlayerPed::m_bytePlayerNumber;
        CScripting::ScriptCommand(&kill_actor, m_dwGTAId);
        CWorld::PlayerInFocus = 0;
    }
}

void CPlayerPed::ExtinguishFire()
{
    if (!CCommon::IsValidGamePed(m_pPed)) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x14E934), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", 8, "__text");
    if (!verified) {
        return;
    }

    uintptr_t pFireObject = reinterpret_cast<uintptr_t>(GetPed()->m_pFire);
    if (pFireObject != 0)
    {
        // CFire::Extinguish
        reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x14E934) + _dyld_get_image_vmaddr_slide(0))(pFireObject);
    }

    if (CPlayerPed::IsInVehicle())
    {
        CVehicleGta* pVeh = GetPed()->pVehicle;
        pFireObject = reinterpret_cast<uintptr_t>(pVeh->m_pFire);
        if (pFireObject)
        {
            // CFire::Extinguish
            reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x14E934) + _dyld_get_image_vmaddr_slide(0))(pFireObject);
        }
    }
}

void CPlayerPed::ApplyAnimation(const char *szAnimName, const char *szAnimFile, float fT, int opt1, int opt2, int opt3, int opt4, int iUnk)
{
    // CNecessary::Log(OBFUSCATE("ApplyAnimation: %s %s %.2f %d %d %d %d %d"), szAnimName, szAnimFile, fT, opt1, opt2, opt3, opt4, iUnk);

	if (!szAnimName || !szAnimFile) {
		return;
	}

	if (!CPlayerPed::VerifyInstance()) {
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

	CScripting::ScriptCommand(&apply_animation, m_dwGTAId, szAnimName, szAnimFile, fT, opt1, opt2, opt3, opt4, iUnk);
}

void CPlayerPed::ClearAnimations()
{
    CNecessary::Log(OBFUSCATE("CPlayerPed ClearAnimations"));

	if (!CPlayerPed::VerifyInstance()) {
		return;
	}

	// CMatrix mat;
	// GetMatrix(&mat);
	// TeleportTo(mat.pos.x, mat.pos.y, mat.pos.z);

	CScripting::ScriptCommand(&disembark_instantly_actor, m_dwGTAId);
}

void* CPlayerPed::GetNamedAnimationTask()
{
    if (!CPlayerPed::VerifyInstance()) {
        return nullptr;
    }

    void* taskMgr = GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY];
    if (!taskMgr) {
        return nullptr;
    }
    
    if (CCommon::GetTaskTypeFromTask(taskMgr) == 401 || CCommon::GetTaskTypeFromTask(taskMgr) == 934) {
        return taskMgr;
    }

    taskMgr = CCommon::GetNextTaskFromTask(taskMgr);
    if (!taskMgr) {
        return nullptr;
    }

    while (CCommon::GetTaskTypeFromTask(taskMgr) != 401 && CCommon::GetTaskTypeFromTask(taskMgr) != 934)
    {
        taskMgr = CCommon::GetNextTaskFromTask(taskMgr);
        if (!taskMgr) {
            return nullptr;
        }
    }

    if (taskMgr && (CCommon::GetTaskTypeFromTask(taskMgr) == 401 || CCommon::GetTaskTypeFromTask(taskMgr) == 934)) {
        return taskMgr;
    }

    return nullptr;
}

int32_t CPlayerPed::GetBlendedAnimationsIndex()
{
    if (!GetPed() || CCommon::IsGameEntityArePlaceable(m_pEntity) || !GetPed()->m_pRwObject) {
        return 0;
    }

    // แก้ไขบัค สำหรับ Mobile อย่างเดียว เพราะเวลาใช้อาวุธพวกนี้แล้ว มันยังส่งค่า -1 มาอยู่ (Sync ภาพตรงพวก WEAPON_SPRAYCAN, WEAPON_FIREEXTINGUISHER, WEAPON_FLAMETHROWER)
    uint8_t currentWeapon = CPlayerPed::GetCurrentWeapon();
    if (currentWeapon == eWeaponSampId::WEAPON_SPRAYCAN || currentWeapon == eWeaponSampId::WEAPON_FIREEXTINGUISHER || currentWeapon == eWeaponSampId::WEAPON_FLAMETHROWER)
    {
        if (CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE])
        {
            uint16_t animId = (currentWeapon == eWeaponSampId::WEAPON_FIREEXTINGUISHER) ? 1168 : 1167;
            int32_t upperBits = (-32760 | 0xFFFF8000) << 16;
            return animId | upperBits;
        }
    }

    void* pAnimTask = CPlayerPed::GetNamedAnimationTask();
    if (pAnimTask)
    {
        return CAnimation::GetIndexFromTask(pAnimTask);
    }

    int32_t animHash = CAnimation::GetAnimationKey(CAnimation::m_AnimGroup, CAnimation::m_AnimId);
    int32_t animIndex = CAnimation::GetAnimationIndexFromKey(animHash);

    uint8_t blendByte = static_cast<uint8_t>(CAnimation::m_BlendDelta);
    int32_t upperBits = (blendByte | 0xFFFF8000) << 16;

    return animIndex | upperBits;
}

void CPlayerPed::PlayAnimationByIndex(uint32_t dwAnimation)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    uint16_t animIndex = static_cast<uint16_t>(dwAnimation & 0xFFFF);
    bool bLoop = (dwAnimation & 0x2000000) != 0;
    bool bLockX = (dwAnimation & 0x4000000) != 0;
    bool bFreeze = (dwAnimation & 0x8000000) != 0;
    bool bBlend = ((dwAnimation >> 24) & 1) != 0;

    float fFramedelta = static_cast<float>((dwAnimation >> 16) & 0xFF);

    int32_t nTimeMode = 0;
    uint8_t nTypeMode = (dwAnimation >> 28) & 3;
    switch (nTypeMode)
    {
        case 1: {
            nTimeMode = -1;
            break;
        }

        case 2: {
            nTimeMode = 1;
            break;
        }

        default: {
            nTimeMode = 0;
            break;
        }
    }

    const char* animationData = CAnimation::RetrieveAnimationByIndex(animIndex - 1);
    if (!animationData) {
        return;
    }

    char szLibrary[32] = { 0 };
    char szAnimName[32] = { 0 };

    const char* pColon = strchr(animationData, ':');
    if (pColon)
    {
        size_t nLibLen = pColon - animationData;
        if (nLibLen < sizeof(szLibrary))
        {
            strncpy(szLibrary, animationData, nLibLen);
            szLibrary[nLibLen] = '\0';
        }

        strncpy(szAnimName, pColon + 1, sizeof(szAnimName) - 1);

        CPlayerPed::ApplyAnimation(szAnimName, szLibrary, fFramedelta, bBlend, bLoop, bLockX, bFreeze, nTimeMode);

        if (!strcmp((const char*)szLibrary, OBFUSCATE("PARACHUTE")))
        {
            CPlayerPed::ProcessParachuteEvent((const char*)szAnimName);
        }
    }
}

void* CPlayerPed::IsPerformingCarryTask()
{
    if (!CPlayerPed::VerifyInstance()) {
        return nullptr;
    }

    void* taskMgr = GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY];
    if (!taskMgr) {
        return nullptr;
    }

    int taskType = CCommon::GetTaskTypeFromTask(taskMgr);
    if (taskType == 401 || taskType == 934) {
        return taskMgr;
    }

    taskMgr = CCommon::GetNextTaskFromTask(taskMgr);
    while (taskMgr)
    {
        taskType = CCommon::GetTaskTypeFromTask(taskMgr);
        if (taskType == 401 || taskType == 934) {
            return taskMgr;
        }

        taskMgr = CCommon::GetNextTaskFromTask(taskMgr);
    }

    return nullptr;
}

bool CPlayerPed::OpenParachute()
{
    if (!CPlayerPed::VerifyInstance()) {
        return false;
    }

    if (!CPlayerPed::m_dwParachuteObject)
    {
        CMatrix mat;
        GetMatrix(&mat);
        CScripting::ScriptCommand(&create_object, eSpecialObject::OBJECT_PARACHUTE, mat.pos.x, mat.pos.y, mat.pos.z, &m_dwParachuteObject);

        if (!CUtil::GamePool_Object_GetAt(CPlayerPed::m_dwParachuteObject))
        {
            CPlayerPed::m_dwParachuteObject = 0;
            return false;
        }

        const auto paraOpen = OBFUSCATE("PARA_OPEN_O");
        const auto paraType = OBFUSCATE("PARACHUTE");

        CScripting::ScriptCommand(&attach_object_to_actor, CPlayerPed::m_dwParachuteObject, m_dwGTAId, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        CScripting::ScriptCommand(&set_object_visible, CPlayerPed::m_dwParachuteObject, 0);
        CScripting::ScriptCommand(&set_object_scale, CPlayerPed::m_dwParachuteObject, 0.0f);
        CScripting::ScriptCommand(&apply_object_animation, CPlayerPed::m_dwParachuteObject, static_cast<const char*>(paraOpen), static_cast<const char*>(paraType), 1000.0f, false, true);
        CScripting::ScriptCommand(&set_object_visible, CPlayerPed::m_dwParachuteObject, 1);
        CScripting::ScriptCommand(&set_object_scale, CPlayerPed::m_dwParachuteObject, 1.0f);
    }

    return true;
}

void CPlayerPed::DestroyParachute()
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (CPlayerPed::m_dwParachuteObject)
    {
        CScripting::ScriptCommand(&disassociate_object, CPlayerPed::m_dwParachuteObject, 0.0, 0.0, 0.0, 0);
        CScripting::ScriptCommand(&destroy_object_with_fade, CPlayerPed::m_dwParachuteObject);
        CPlayerPed::m_dwParachuteObject = 0;
    }
}

void CPlayerPed::ProcessParachuteEvent(const char* szAnimName)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (!szAnimName || !CNecessary::MPSTRLEN(szAnimName)) {
        return;
    }

    if (CPlayerPed::GetCurrentWeapon() == eWeaponSampId::WEAPON_PARACHUTE)
    {
        const auto paraType = OBFUSCATE("PARACHUTE");

        if (!strcasecmp(szAnimName, OBFUSCATE("PARA_OPEN")))
        {
            if (CPlayerPed::OpenParachute())
            {
                CMatrix matPos;
                GetMatrix(&matPos);

                CPedGta* pPed = CUtil::GamePool_FindPlayerPed();
                if (pPed)
                {
                    CVector playerPos;
                    playerPos.x = pPed->mat->pos.x;
                    playerPos.y = pPed->mat->pos.y;
                    playerPos.z = pPed->mat->pos.z;

                    CVector soundPos;
                    soundPos.x = playerPos.x + (matPos.pos.x - playerPos.x) * 0.1f;
                    soundPos.y = playerPos.y + (matPos.pos.y - playerPos.y) * 0.1f;
                    soundPos.z = playerPos.z + (matPos.pos.z - playerPos.z) * 0.1f;
                    CScripting::ScriptCommand(&play_sound, soundPos.x, soundPos.y, soundPos.z, 1039);
                }
            }
        }
        else if (!strcasecmp(szAnimName, OBFUSCATE("PARA_DECEL")))
        {
            if (CPlayerPed::OpenParachute())
            {
                const auto paraDecel = OBFUSCATE("PARA_DECEL_O");

                CScripting::ScriptCommand(&apply_object_animation, CPlayerPed::m_dwParachuteObject, static_cast<const char*>(paraDecel), static_cast<const char*>(paraType), 1.0f, true, true);
            }
        }
        else if (!strcasecmp(szAnimName, OBFUSCATE("PARA_FLOAT")))
        {
            if (CPlayerPed::OpenParachute())
            {
                const auto paraFloat = OBFUSCATE("PARA_FLOAT_O");

                CScripting::ScriptCommand(&apply_object_animation, CPlayerPed::m_dwParachuteObject, static_cast<const char*>(paraFloat), static_cast<const char*>(paraType), 1.0f, true, true);
            }
        }
        else if (!strcasecmp(szAnimName, OBFUSCATE("PARA_STEERR")))
        {
            if (CPlayerPed::OpenParachute())
            {
                const auto paraSteerr = OBFUSCATE("PARA_STEERR_O");

                CScripting::ScriptCommand(&apply_object_animation, CPlayerPed::m_dwParachuteObject, static_cast<const char*>(paraSteerr), static_cast<const char*>(paraType), 1.0f, true, true);
            }
        }
        else if (!strcasecmp(szAnimName, OBFUSCATE("PARA_STEERL")))
        {
            if (CPlayerPed::OpenParachute())
            {
                const auto paraSteerl = OBFUSCATE("PARA_STEERL_O");

                CScripting::ScriptCommand(&apply_object_animation, CPlayerPed::m_dwParachuteObject, static_cast<const char*>(paraSteerl), static_cast<const char*>(paraType), 1.0f, true, true);
            }
        }
        else if (!strcasecmp(szAnimName, OBFUSCATE("PARA_LAND_WATER")))
        {
            if (CPlayerPed::OpenParachute())
            {
                const auto paraLandWater = OBFUSCATE("PARA_LAND_WATER_O");

                CScripting::ScriptCommand(&apply_object_animation, CPlayerPed::m_dwParachuteObject, static_cast<const char*>(paraLandWater), static_cast<const char*>(paraType), 1.0f, false, true);
            }
        }
    }
}

void CPlayerPed::ApplyCrouch()
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (!(GetPed()->bIsDucking))
    {
        if (!CPlayerPed::IsCrouching())
        {
            if (GetPed()->pPedIntelligence)
            {
                CPedIntelligence::SetTaskDuckSecondary(GetPed()->pPedIntelligence, 0);
            }
        }
    }
}

void CPlayerPed::ResetCrouch()
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    GetPed()->bIsDucking = 0;
    if (GetPed()->pPedIntelligence)
    {
        CPedIntelligence::ClearTaskDuckSecondary(GetPed()->pPedIntelligence);
    }
}

bool CPlayerPed::IsCrouching()
{
    if (!CPlayerPed::VerifyInstance()) {
        return false;
    }

    if (!CUtil::GamePool_Ped_GetAt(m_dwGTAId))
    {
        return false;
    }

    return GetPed()->bIsDucking;
}

void CPlayerPed::RemoveWeaponWhenEnteringVehicle()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2434F8), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9\xFD\x43\x00\x91", 12, "__text");
    if (!verified || !CPlayerPed::VerifyInstance()) {
        return;
    }

    reinterpret_cast<void (*)(CPedGta*, int)>(SA_ADDR(0x2434F8) + _dyld_get_image_vmaddr_slide(0))(GetPed(), 0);
}

void CPlayerPed::ProcessCuffedControl()
{
    if (!CPlayerPed::VerifyInstance() || !CPlayerPed::IsCuffed()) {
        return;
    }

    int iWeaponID = 0;

    CScripting::ScriptCommand(&get_actor_armed_weapon, m_dwGTAId, &iWeaponID);

    if (iWeaponID)
    {
        CPlayerPed::SetArmedWeapon(0);
    }
}

void CPlayerPed::ProcessCuffed()
{
    if (!CPlayerPed::VerifyInstance() || !CPlayerPed::IsCuffed()) {
        return;
    }

    if (CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    int boneIds[] = {
        eBoneTag::BONE_L_UPPER_ARM,
        eBoneTag::BONE_L_FORE_ARM,
        eBoneTag::BONE_L_HAND,
        eBoneTag::BONE_R_UPPER_ARM,
        eBoneTag::BONE_R_FORE_ARM,
        eBoneTag::BONE_R_HAND
    };

    float rotations[][3] = {
        {26.57374382019f, 61.337575733622f, 59.206573486328f},  // L_UPPER_ARM
        {27.843754291534f, 15.3639249801636f, 46.40625f},       // L_FORE_ARM
        {-81.018516340527f, 342.87482380867f, 326.11833715439f}, // L_HAND
        {338.839179039f, 53.49357098341f, 298.45233917236f},    // R_UPPER_ARM
        {307.68748283386f, 22.110015869141f, 313.59375f},       // R_FORE_ARM
        {96.047592163086f, 357.88313293457f, 56.739406585693f}  // R_HAND
    };

    if (GetPed()->nModelIndex != 0)
    {
        for (int i = 0; i < 6; i++)
        {
            CAnimBlendClumpData** clumpDataPtr = CCommon::GetClumpData(GetPed()->m_pRwClump);
            if (!clumpDataPtr || !*clumpDataPtr) {
                return;
            }

            CAnimBlendClumpData* clumpData = *clumpDataPtr;
            AnimBlendFrameData* frameData = clumpData->GetFrameDataByNodeId(boneIds[i]);
            if (frameData)
            {
                CQuaternion* tempOrientation = &frameData->GetFrameOrientation();
                RtQuat* boneOrientation = reinterpret_cast<RtQuat*>(tempOrientation);
                RwV3d angles = {rotations[i][0], rotations[i][2], rotations[i][1]};
                BoneNode_c::EulerToQuat(&angles, boneOrientation);
                GetPed()->nEntityFlags.m_bDontUpdateHierarchy = false;
            }
        }
    }

    CEntity::UpdateRpHAnim(reinterpret_cast<CPedGta*>(GetPed()));
}

void CPlayerPed::ToggleCuffed(bool cuff)
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded()) {
        return;
    }

    CPlayerPed::m_bCuffedState = cuff;
    CPlayerPed::m_bSprintControl = cuff != 0;
}

void CPlayerPed::ProcessCarryControl()
{
    if (!CPlayerPed::VerifyInstance() || !CPlayerPed::IsCarry()) {
        return;
    }

    int iWeaponID = 0;

    CScripting::ScriptCommand(&get_actor_armed_weapon, m_dwGTAId, &iWeaponID);

    if (iWeaponID)
    {
        CPlayerPed::SetArmedWeapon(0);
    }
}

void CPlayerPed::ProcessCarry()
{
    if (!CPlayerPed::VerifyInstance() || !CPlayerPed::IsCarry()) {
        return;
    }

    if (CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    RpClump* pRwClump = GetPed()->m_pRwClump;
    if (pRwClump)
    {
        RpHAnimHierarchy* hierarchy = CCommon::GetAnimHierarchyFromSkinClump(reinterpret_cast<uintptr_t*>(pRwClump));

        if (hierarchy && hierarchy->currentAnim)
        {
            RtAnimInterpolator* interpolator = hierarchy->currentAnim;
            int nNodeSize = interpolator->currentInterpKeyFrameSize;
            uint8_t* pQuatBase = reinterpret_cast<uint8_t*>(interpolator + 1);

            auto SetBoneQuat = [&](int nIndex, float x, float y, float z, float w) {
                float* pDest = reinterpret_cast<float*>(pQuatBase + (nIndex * nNodeSize));
                pDest[0] = x; pDest[1] = y; pDest[2] = z; pDest[3] = w;
            };

            // แขนซ้าย (L_ARM) Dump มาจาก SAMP PC ตรงกันแบบ 100%
            SetBoneQuat(9, -0.6432f, -0.0740f, -0.7482f, -0.1446f); // Shoulder L (Tag 31)
            SetBoneQuat(10, -0.1306f, 0.5489f, -0.1104f, 0.8182f); // L_UPPER (Tag 32)
            SetBoneQuat(11, 0.0000f, 0.0000f, -0.6211f, 0.7837f);  // L_FORE (Tag 33)
            SetBoneQuat(12, -0.9869f, -0.0742f, 0.0269f, -0.1409f); // L_HAND (Tag 34)
            SetBoneQuat(13, -0.0002f, -0.0000f, 0.1695f, 0.9855f); // L_Finger (Tag 35)
            SetBoneQuat(14, 0.0000f, 0.0000f, 0.1162f, 0.9932f);  // L_Finger1 (Tag 36)

            // แขนขวา (R_ARM) Dump มาจาก SAMP PC ตรงกันแบบ 100%
            SetBoneQuat(15, 0.6473f, 0.0884f, -0.7404f, -0.1580f); // Shoulder R (Tag 21)
            SetBoneQuat(16, 0.1783f, -0.5777f, -0.0679f, 0.7936f);  // R_UPPER (Tag 22)
            SetBoneQuat(17, 0.0000f, 0.0000f, -0.5709f, 0.8210f);  // R_FORE (Tag 23)
            SetBoneQuat(18, -0.9629f, -0.0369f, -0.0142f, 0.2669f); // R_HAND (Tag 24)
            SetBoneQuat(19, 0.0002f, 0.0000f, 0.1695f, 0.9855f);  // R_Finger (Tag 25)
            SetBoneQuat(20, 0.0000f, 0.0000f, 0.1162f, 0.9932f);  // R_Finger1 (Tag 26)
        }
    }

    CEntity::UpdateRpHAnim(reinterpret_cast<CPedGta*>(GetPed()));
}

void CPlayerPed::ToggleCarry(bool carry)
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded()) {
        return;
    }

    CPlayerPed::m_bCarryState = carry;
    if (CPlayerPed::m_bCarryState)
    {
        CPlayerPed::m_bJumpControl = true;
        CPlayerPed::m_bCrouchControl = true;
        CPlayerPed::m_bSprintControl = true;
    }
    else
    {
        CPlayerPed::m_bJumpControl = false;
        CPlayerPed::m_bCrouchControl = false;
        CPlayerPed::m_bSprintControl = false;
    }
}

void CPlayerPed::StartPissing()
{
    if (!CPlayerPed::VerifyInstance() || CPlayerPed::IsInVehicle() || !IsAdded()) {
        return;
    }

    if (!CPlayerPed::IsPissing())
    {
        CPlayerPed::ApplyAnimation(OBFUSCATE("PISS_LOOP"), OBFUSCATE("PAULNMAC"), 4.0f, 1, 0, 0, 0, -1);

        const auto petrolcan = OBFUSCATE("PETROLCAN");
        CScripting::ScriptCommand(&attach_particle_to_actor2, static_cast<const char*>(petrolcan), m_dwGTAId, 0.0f, 0.58f, -0.08f, 0.0f, 0.01f, 0.0f, 1, &m_dwPissParticlesHandle);
        CScripting::ScriptCommand(&make_particle_visible, m_dwPissParticlesHandle);

        CPlayerPed::m_bPissingState = true;
    }
}

void CPlayerPed::StopPissing()
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded()) {
        return;
    }

    if (CPlayerPed::IsPissing())
    {
        if (CPlayerPed::m_dwPissParticlesHandle)
        {
            CScripting::ScriptCommand(&destroy_particle, m_dwPissParticlesHandle);
            CPlayerPed::m_dwPissParticlesHandle = 0;
        }

        CMatrix mat;
        GetMatrix(&mat);
        TeleportTo(mat.pos.x, mat.pos.y, mat.pos.z);

        CPlayerPed::m_bPissingState = false;
    }
}

void CPlayerPed::ToggleCellphone(int enable)
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded()) {
        return;
    }

    CPlayerPed::m_iCellPhoneEnabled = enable;
    CScripting::ScriptCommand(&toggle_actor_cellphone, m_dwGTAId, enable);
}

void CPlayerPed::HandsUp()
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded()) {
        return;
    }

    CScripting::ScriptCommand(&actor_task_handsup, m_dwGTAId, -2);
}

bool CPlayerPed::HasHandsUp()
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded()) {
        return false;
    }

    CTask* taskMgr = GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY];
    if (!taskMgr) {
        return false;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x10AF0C), "\x88\x1C\x00\xB0\x08\xE1\x02\x91\x08\x41\x00\x91\xA8\x02\x00\xF9", 16, "__text");
    uintptr_t taskVtbl = *reinterpret_cast<uintptr_t*>(taskMgr);
    if (verified && taskVtbl == SA_ADDR(0x49B0C8) + _dyld_get_image_vmaddr_slide(0))
    {
        return true; // CTaskSimpleHandsUp vtable ของ iOS
    }

    return false;
}

void CPlayerPed::ApplyCommandTask(const char* a2, CEntityGta* a4, int a5, int a6, CVector* a7, char a8, float a9, int a10, int a11, char a12)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x2D1F94), "\xFF\x43\x02\xD1\xEB\x2B\x01\x6D\xE9\x23\x02\x6D\xFC\x6F\x03\xA9", 16, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x11F108), "\x40\x37\x00\xB0\x00\x80\x1A\x91", 8, "__text");
    if (!verified) {
        return;
    }

    // IKChainManager_c::LookAt
    const auto slide = _dyld_get_image_vmaddr_slide(0);
    reinterpret_cast<void (*)(uintptr_t, const char*, CPedGta*, CEntityGta*, int, int, CVector*, uint8_t, float, int, int, uint8_t)>(SA_ADDR(0x2D1F94) + slide)(SA_ADDR(0x8086A0) + slide, a2, GetPed(), a4, a5, a6, a7, a8, a9, a10, a11, a12);
}

void CPlayerPed::SetDrunkLevel(uint32_t dwLevel)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (dwLevel > 50000)
    {
        dwLevel = 50000;
    }

    CPlayerPed::m_stuffData.dwDrunkLevel = dwLevel;
}

void CPlayerPed::ProcessDrunk()
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    int iDrunkLevel = CPlayerPed::GetDrunkLevel();
    if (iDrunkLevel == 0) {
        CScripting::ScriptCommand(&set_player_drunk_visuals, 0, 0);
        return;
    }

    if (!CPlayerPed::m_bytePlayerNumber)
    {
        int iDrunkVisual = 0;

        if (iDrunkLevel > 0 && iDrunkLevel <= 2000)
        {
            CPlayerPed::SetDrunkLevel(iDrunkLevel - 1);
            iDrunkVisual = 0;
        }
        else if (iDrunkLevel > 2000)
        {
            iDrunkVisual = static_cast<int>(iDrunkLevel * 0.02);
            if (iDrunkVisual <= 250)
            {
                if (iDrunkVisual < 5)
                {
                    iDrunkVisual = 0;
                }
            }
            else
            {
                iDrunkVisual = 250;
            }

            // Vehicle
            if (CPlayerPed::IsInVehicle() && !CPlayerPed::IsAPassenger())
            {
                CVehicleGta* pVehicle = CPlayerPed::GetVehicle();
                if (pVehicle)
                {
                    if (!CPlayerPed::m_stuffData.dwLastUpdateTick || (CNecessary::GetTickCount() - CPlayerPed::m_stuffData.dwLastUpdateTick) > 200)
                    {
                        int iRandNumber = rand() % 40;
                        float fRotation = 0.0f;
                        if (iRandNumber >= 20)
                        {
                            fRotation = 0.012f;
                            if (iDrunkLevel >= 5000)
                            {
                                fRotation = 0.015f;
                            }

                            if (iRandNumber <= 30)
                            {
                                fRotation = -0.012f;
                                if (iDrunkLevel >= 5000)
                                {
                                    fRotation = -0.015f;
                                }
                            }
                        }

                        if (CUtil::FloatOffset(pVehicle->vecMoveSpeed.x, 0.0f) > 0.050000001f || CUtil::FloatOffset(pVehicle->vecMoveSpeed.y, 0.0f) > 0.050000001f)
                        {
                            pVehicle->m_vecTurnSpeed.z = fRotation + pVehicle->m_vecTurnSpeed.z;
                        }

                        CPlayerPed::m_stuffData.dwLastUpdateTick = CNecessary::GetTickCount();
                    }
                }
            }

            CPlayerPed::SetDrunkLevel(iDrunkLevel - 1);
        }

        CScripting::ScriptCommand(&set_player_drunk_visuals, CPlayerPed::m_bytePlayerNumber, iDrunkVisual);
    }
}

void CPlayerPed::SetDrunkVisuals(uint32_t dwVisuals)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    CScripting::ScriptCommand(&set_player_drunk_visuals, 0, dwVisuals);
}

void CPlayerPed::SetDrunkHandling(uint32_t dwHandling)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    CScripting::ScriptCommand(&set_player_drunk_handling, 0, dwHandling);
}

CVehicleGta* CPlayerPed::GetVehicle()
{
    return (CVehicleGta*)GetPed()->pVehicle;
}

void CPlayerPed::ApplyStuff()
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (CPlayerPed::GetStuff() && GetPed() && IsAdded())
    {
        PAD_KEYS *pad;

        if (!CPlayerPed::m_bytePlayerNumber)
        {
            pad = &CPad::LocalPlayerKeys;
        }
        else
        {
            pad = &CPad::RemotePlayerKeys[CPlayerPed::m_bytePlayerNumber];
        }

        CPlayerPed::SetArmedWeapon(0);

        if (!CPlayerPed::m_bytePlayerNumber && !CPlayerPed::IsPerformingCustomAnim() && pad->bKeys[ePadKeys::KEY_FIRE])
        {
            eStuffType stuffType = CPlayerPed::GetStuff();
            if (stuffType == eStuffType::STUFF_TYPE_BEER || stuffType == eStuffType::STUFF_TYPE_DYN_BEER || stuffType == eStuffType::STUFF_TYPE_PINT_GLASS)
            {
                if (CPlayerPed::GetPedStat() == 5 || CPlayerPed::GetPedStat() == 22)
                {
                    CPlayerPed::ApplyAnimation(OBFUSCATE("DNK_STNDF_LOOP"), OBFUSCATE("BAR"), 4.0, 0, 0, 0, 0, -1);
                }
                else
                {
                    CPlayerPed::ApplyAnimation(OBFUSCATE("DNK_STNDM_LOOP"), OBFUSCATE("BAR"), 4.0, 0, 0, 0, 0, -1);
                }

                if (stuffType != eStuffType::STUFF_TYPE_PINT_GLASS)
                {
                    CPlayerPed::SetDrunkLevel(CPlayerPed::GetDrunkLevel() + 1250);
                }
            }
            else if (stuffType == eStuffType::STUFF_TYPE_CIGGI)
            {
                CPlayerPed::ApplyAnimation(OBFUSCATE("smkcig_prtl"), OBFUSCATE("GANGS"), 700.0, 0, 0, 0, 0, 2750);
            }
        }
    }
}

void CPlayerPed::GiveStuff(eStuffType type)
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (CPlayerPed::IsInVehicle()) {
        return;
    }

    if (CPlayerPed::m_stuffData.dwObject) {
        CPlayerPed::DropStuff();
    }

    CPlayerPed::SetArmedWeapon(0);

    CMatrix matPlayer;
    GetMatrix(&matPlayer);

    switch (type)
    {
        case eStuffType::STUFF_TYPE_BEER: {
            CScripting::ScriptCommand(&create_object, eSpecialObject::OBJECT_CJ_BEER_B_2, matPlayer.pos.x, matPlayer.pos.y, matPlayer.pos.z, &m_stuffData.dwObject);

            if (CUtil::GamePool_Object_GetAt(CPlayerPed::m_stuffData.dwObject))
            {
                CScripting::ScriptCommand(&task_pick_up_object, m_dwGTAId, m_stuffData.dwObject, 0.05000000074505806, 0.02999999932944775, -0.300000011920929, 6, 16, "NULL", "NULL", -1);
            }

            break;
        }

        case eStuffType::STUFF_TYPE_DYN_BEER: {
            CScripting::ScriptCommand(&create_object, eSpecialObject::OBJECT_DYN_BEER_1, matPlayer.pos.x, matPlayer.pos.y, matPlayer.pos.z, &m_stuffData.dwObject);

            if (CUtil::GamePool_Object_GetAt(CPlayerPed::m_stuffData.dwObject))
            {
                CScripting::ScriptCommand(&task_pick_up_object, m_dwGTAId, m_stuffData.dwObject, 0.05000000074505806, 0.02999999932944775, -0.05000000074505806, 6, 16, "NULL", "NULL", -1);
            }

            break;
        }

        case eStuffType::STUFF_TYPE_PINT_GLASS: {
            CScripting::ScriptCommand(&create_object, eSpecialObject::OBJECT_CJ_PINT_GLASS, matPlayer.pos.x, matPlayer.pos.y, matPlayer.pos.z, &m_stuffData.dwObject);

            if (CUtil::GamePool_Object_GetAt(CPlayerPed::m_stuffData.dwObject))
            {
                CScripting::ScriptCommand(&task_pick_up_object, m_dwGTAId, m_stuffData.dwObject, 0.03999999910593033, 0.1000000014901161, -0.01999999955296516, 6, 16, "NULL", "NULL", -1);
            }

            break;
        }

        case eStuffType::STUFF_TYPE_CIGGI: {
            CScripting::ScriptCommand(&create_object, eSpecialObject::OBJECT_CJ_CIGGY, matPlayer.pos.x, matPlayer.pos.y, matPlayer.pos.z, &m_stuffData.dwObject);

            if (CUtil::GamePool_Object_GetAt(CPlayerPed::m_stuffData.dwObject))
            {
                CScripting::ScriptCommand(&task_pick_up_object, m_dwGTAId, m_stuffData.dwObject, 0.0, 0.0, 0.0, 6, 16, "NULL", "NULL", -1);
            }

            break;
        }

        default: {
            break;
        }
    }

    CPlayerPed::m_stuffData.type = type;
}

void CPlayerPed::DropStuff()
{
    if (!CPlayerPed::VerifyInstance()) {
        return;
    }

    if (CUtil::GamePool_Object_GetAt(CPlayerPed::m_stuffData.dwObject))
    {
        CScripting::ScriptCommand(&task_pick_up_object, m_dwGTAId, m_stuffData.dwObject, 0.0, 0.0, 0.0, 6, 16, "NULL", "NULL", 0);
        CPlayerPed::m_stuffData.dwObject = 0;
    }

    CMatrix matPlayer;
    GetMatrix(&matPlayer);
    TeleportTo(matPlayer.pos.x, matPlayer.pos.y, matPlayer.pos.z);

    CPlayerPed::m_stuffData.type = eStuffType::STUFF_TYPE_NONE;
}

eStuffType CPlayerPed::GetStuff()
{
    if (!CPlayerPed::VerifyInstance()) {
        return eStuffType::STUFF_TYPE_NONE;
    }

    return CPlayerPed::m_stuffData.type;
}

bool CPlayerPed::IsPlayingAnimation(const char* szAnimName)
{
    if (!CPlayerPed::VerifyInstance()) {
        return false;
    }

    if (!szAnimName || !CNecessary::MPSTRLEN(szAnimName)) {
        return false;
    }

    if (CScripting::ScriptCommand(&is_char_playing_anim, m_dwGTAId, szAnimName)) {
        return true;
    }

    return false;
}

void CPlayerPed::StartDancing(int danceId)
{
    if (!CPlayerPed::VerifyInstance() || CPlayerPed::IsInVehicle() || !IsAdded()) {
        return;
    }

    if (danceId < 0 || danceId > 3) {
        return;
    }

    CPlayerPed::m_bDanceState = true;
    CPlayerPed::m_iDanceStyle = danceId;

    CPlayerPed::ApplyAnimation(DanceIdleLoops[CPlayerPed::m_iDanceStyle], DanceStyleLibs[CPlayerPed::m_iDanceStyle], 16.0f, 1, 0, 0, 0, -1);
}

void CPlayerPed::StopDancing()
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded()) {
        return;
    }

    CPlayerPed::m_bDanceState = false;
    CPlayerPed::m_iDanceStyle = -1;

    if (GetPed() == CUtil::GamePool_FindPlayerPed())
    {
        CGame::DisplayHUD(true);
    }

    CMatrix mat;
    GetMatrix(&mat);
    TeleportTo(mat.pos.x, mat.pos.y, mat.pos.z);
}

void CPlayerPed::ProcessDancing()
{
    if (!CPlayerPed::VerifyInstance() || !IsAdded() || !CPlayerPed::IsDancing() || CPlayerPed::m_bytePlayerNumber) {
        return;
    }

    uint16_t wKeysUpDown = 0, wKeysLeftRight = 0, iExtra = 0;
    if (!CPlayerPed::m_bytePlayerNumber)
    {
        wKeysUpDown = CPad::LocalPlayerKeys.wKeyUD;
        wKeysLeftRight = CPad::LocalPlayerKeys.wKeyLR;
        iExtra = CPad::LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT];
    }

    if (!CPlayerPed::IsPerformingCustomAnim())
    {
        CPlayerPed::ApplyAnimation(DanceIdleLoops[m_iDanceStyle], DanceStyleLibs[m_iDanceStyle], 4.0f, 1, 0, 0, 0, 0);
    }

    int iNewMove = 0;
    if (wKeysUpDown > 128 && !wKeysLeftRight && !iExtra)
    {
        iNewMove = 0; // UP
    }
    else if (wKeysUpDown < 128 && !wKeysLeftRight && !iExtra)
    {
        iNewMove = 1; // DOWN
    }
    else if (!wKeysUpDown && wKeysLeftRight < 128 && !iExtra)
    {
        iNewMove = 2; // LEFT
    }
    else if (!wKeysUpDown && wKeysLeftRight > 128 && !iExtra)
    {
        iNewMove = 3; // RIGHT
    }
    else if (wKeysUpDown > 128 && wKeysLeftRight < 128 && !iExtra)
    {
        iNewMove = 4; // UP-LEFT
    }
    else if (wKeysUpDown > 128 && wKeysLeftRight > 128 && !iExtra)
    {
        iNewMove = 5; // UP-RIGHT
    }
    else if (wKeysUpDown < 128 && wKeysLeftRight < 128 && !iExtra)
    {
        iNewMove = 6; // DOWN-LEFT
    }
    else if (wKeysUpDown < 128 && wKeysLeftRight > 128 && !iExtra)
    {
        iNewMove = 7; // DOWN-RIGHT
    }
    else if (wKeysUpDown > 128 && !wKeysLeftRight && iExtra)
    {
        iNewMove = 8; // UP/EX
    }
    else if (wKeysUpDown < 128 && !wKeysLeftRight && iExtra)
    {
        iNewMove = 9; // DOWN/EX
    }
    else if (!wKeysUpDown && wKeysLeftRight < 128 && iExtra)
    {
        iNewMove = 10; // LEFT/EX
    }
    else if (!wKeysUpDown && wKeysLeftRight > 128 && iExtra)
    {
        iNewMove = 11; // RIGHT/EX
    }
    else if (wKeysUpDown > 128 && wKeysLeftRight < 128 && iExtra)
    {
        iNewMove = 12; // UP-LEFT/EX
    }
    else if (wKeysUpDown > 128 && wKeysLeftRight > 128 && iExtra)
    {
        iNewMove = 13; // UP-RIGHT/EX
    }
    else if (wKeysUpDown < 128 && wKeysLeftRight < 128 && iExtra)
    {
        iNewMove = 14; // DOWN-LEFT/EX
    }
    else if (wKeysUpDown < 128 && wKeysLeftRight > 128 && iExtra)
    {
        iNewMove = 15; // DOWN-RIGHT/EX
    }
    else
    {
        return;
    }

    if (iNewMove == CPlayerPed::m_iLastDanceMove) {
        return;
    }

    CPlayerPed::m_iLastDanceMove = iNewMove;

    const char* szAnimName = CPlayerPed::GetDanceAnimForMove(iNewMove);
    if (!szAnimName && !CNecessary::MPSTRLEN(szAnimName)) {
        return;
    }

    CPlayerPed::ApplyAnimation(szAnimName, DanceStyleLibs[m_iDanceStyle], 4.0f, 0, 0, 0, 0, -1);
}

const char* CPlayerPed::GetDanceAnimForMove(int iMove)
{
    if (static_cast<int>(!CPlayerPed::m_iDanceStyle) == -1) {
        return "";
    }

    if (CPlayerPed::m_iDanceStyle >= 0 && CPlayerPed::m_iDanceStyle <= 2)
    {
        if (CPlayerPed::GetPedStat() == 5 || CPlayerPed::GetPedStat() == 22)
        {
            return szDanceAnimNamesFemale[iMove];
        }

        return szDanceAnimNamesMale[iMove];
    }
    else if (CPlayerPed::m_iDanceStyle == 3)
    {
        return szStripAnims[iMove];
    }

    return "";
}

int CPlayerPed::GetPedStat()
{
    if (!CPlayerPed::VerifyInstance()) {
        return -1;
    }

    return CUtil::GamePedStatPrim(m_pPed->nModelIndex);
}

bool CPlayerPed::IsPerformingCustomAnim()
{
    if (!CPlayerPed::VerifyInstance()) {
        return false;
    }

    if (!IsAdded()) {
        return false;
    }

    if (GetPed()->pPedIntelligence->m_taskManager.m_tasks[ePrimaryTasks::TASK_PRIMARY_PRIMARY])
    {
        return true;
    }

    return false;
}