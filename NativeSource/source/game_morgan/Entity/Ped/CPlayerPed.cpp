#include "CPlayerPed.h"
#include "CPed.h"
#include "CPlayerPedSlot.h"
#include "CPedPointerRecorder.h"
#include "../../CGame.h"
#include "../../CUtil.h"
#include "../../CCommon.h"
#include "../../CStreaming.h"
#include "../../Scripts/CScripting.h"
#include "../../../utils/Necessary/CNecessary.h"

#include <mach-o/dyld.h>

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