#include "CPlayerPed.h"
#include "CPed.h"
#include "CPlayerPedSlot.h"
#include "CPedPointerRecorder.h"
#include "../../CGame.h"
#include "../../CUtil.h"
#include "../../CWorld.h"
#include "../../CCommon.h"
#include "../../CStreaming.h"
#include "../../CWeapon.h"
#include "../../Enums/eSpecialVehicle.h"
#include "../../Enums/eWeaponSampid.h"
#include "../../Scripts/CScripting.h"
#include "../../../utils/Necessary/CNecessary.h"

#include <mach-o/dyld.h>
#include <unistd.h>

bool CPlayerPed::m_bRebuildPlayer = true;

CPlayerPed::CPlayerPed()
{
    //CNecessary::Log(OBFUSCATE("CPlayerPed (Line: %d)"), __LINE__);

    m_dwGTAId = 1;
    m_pPed = nullptr;
    m_pEntity = nullptr;

    m_pPed = CUtil::GamePool_FindPlayerPed();
    m_pEntity = m_pPed;

    CPlayerPed::m_bytePlayerNumber = 0;

    CPedPointerRecorder::Record(CPlayerPed::m_bytePlayerNumber, reinterpret_cast<uintptr_t>(m_pPed));
    CPlayerPedSlot::Set(CPlayerPed::m_bytePlayerNumber, 1);

    CScripting::ScriptCommand(&set_actor_weapon_droppable, static_cast<int>(m_dwGTAId), 1);
    CScripting::ScriptCommand(&set_actor_can_be_decapitated, static_cast<int>(m_dwGTAId), 0);
}

CPlayerPed::CPlayerPed(uint8_t /*bytePlayerNumber*/, int /*iSkin*/, float /*fX*/, float /*fY*/, float /*fZ*/, float /*fRotation*/)
{
    // CNecessary::Log(OBFUSCATE("CPlayerPed (Line: %d): bytePlayerNumber: %d, iSkin: %d, XYZ: %.2f %.2f %.2f, fRotation: %.2f"), __LINE__, bytePlayerNumber, iSkin, fX, fY, fZ, fRotation);    
}

CPlayerPed::~CPlayerPed()
{
    CNecessary::Log(OBFUSCATE("CPlayerPed Uninitialise"));
}

bool CPlayerPed::VerifyInstance()
{
    if (m_pPed && CCommon::IsValidGamePed(m_pPed) && CUtil::GamePool_Ped_GetAt(static_cast<int>(m_dwGTAId)) == m_pPed)
    {
        return true;
    }

    return false;
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
    const auto pedIntelligence = reinterpret_cast<uintptr_t>(GetPed()->pPedIntelligence);
    uintptr_t ikManager = *reinterpret_cast<uintptr_t*>(pedIntelligence + 0x58); // TASK_SECONDARY_IK

    if (ikManager && *reinterpret_cast<const uintptr_t*>(ikManager) == SA_ADDR(0x4AB8B8) + slide)
    {
        reinterpret_cast<void (*)(uintptr_t)>(SA_ADDR(0x360F5C) + slide)(ikManager);
        *reinterpret_cast<uintptr_t*>(pedIntelligence + 0x58) = 0;
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

void CPlayerPed::RemoveFromVehicleAndPutAt(float fX, float fY, float fZ)
{
    if (CPlayerPed::IsInVehicle())
    {
        CScripting::ScriptCommand(&remove_actor_from_car_and_put_at, static_cast<int>(m_dwGTAId), fX, fY, fZ);
    }
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