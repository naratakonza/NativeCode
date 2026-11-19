#include "../../Main.h"
#include "Multiplayer/Syncs/Core/CCheckPoint.h"
#include "CGame.h"
#include "CWorld.h"
#include "CTimer.h"
#include "CStreaming.h"
#include "Entity/Ped/CPedPointerRecorder.h"
#include "Entity/Ped/CPlayerPedSlot.h"
#include "../../Launcher/MGStartupTickGate.h"

#include <mach-o/dyld.h>

CCamera* CGame::m_pGameCamera = (CCamera*)nullptr;
CPlayerPed* CGame::m_pGamePlayer = nullptr;
CModelSnapshot* CGame::m_pModelSnapshot = nullptr;

void CGame::Initialize()
{
	CNecessary::Log(OBFUSCATE("CGame Initialize"));

    CCheckPoint::Initialise();
}

void CGame::Uninitialise()
{
	CNecessary::Log(OBFUSCATE("CGame Uninitialise"));

	SAFE_DELETE(CGame::m_pGameCamera);
    SAFE_DELETE(CGame::m_pModelSnapshot);
    
	if (CGame::m_pGamePlayer)
	{
		CPedPointerRecorder::Record(0, 0);
		CPlayerPedSlot::Set(0, 0);
		SAFE_DELETE(CGame::m_pGamePlayer);
	}
}

void CGame::SetFramerate(std::uint8_t fps)
{
    int mode = 0;
    int frameLimit = 60;
    if (fps == 120 || fps == 2)
    {
        mode = 2;
        frameLimit = 120;
    }
    else if (fps == 90 || fps == 1)
    {
        mode = 1;
        frameLimit = 90;
    }
    else if (fps == 30)
    {
        mode = 3;
        frameLimit = 30;
    }

    CMemory::makeNOP(SA_ADDR(0x43AF04), 1); // ให้ iOS Display Link เป็นตัวกำหนดจังหวะเฟรมของเกม (ช่วยให้ลื่นขึ้น)

    if (MGSetGtaFramerate(frameLimit))
    {
        CNecessary::Log(OBFUSCATE("CGame::SetFramerate: mode %d, preferred FPS %d"), mode, frameLimit);
    }
    else
    {
        CNecessary::Log(OBFUSCATE("CGame::SetFramerate: GTA iOS display link hook unavailable"));
    }
}

uint8_t CGame::IsMenuActive() 
{
    return CTimer::IsUserPause() == 1;
}

bool CGame::IsStarted()
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x47F4C), "\x49\x3A\x00\x90\x29\xA1\x07\x91\x29\xE5\x42\x39", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x43AC60), "\xA8\x1A\x00\xB0\x08\xA1\x07\x91\x1F\xE5\x02\x39", 12, "__text");
    if (!verified) {
        return false;
    }

    return *reinterpret_cast<const std::uint8_t*>(SA_ADDR(0x78F2A1) + _dyld_get_image_vmaddr_slide(0)) == 0;
}

bool CGame::IsRadarActive()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x109F40), "\x2A\x35\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x109F44), "\x4A\xB1\x0B\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x109F48), "\x49\x01\x00\x39");
    return verified && *reinterpret_cast<const std::uint8_t*>(SA_ADDR(0x7AE2EC) + _dyld_get_image_vmaddr_slide(0)) == 1;
}

void CGame::ToggleRadar(bool iToggle)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x109F40), "\x2A\x35\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x109F44), "\x4A\xB1\x0B\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x109F48), "\x49\x01\x00\x39");
    if (!verified) {
        return;
    }

    *reinterpret_cast<std::uint8_t*>(SA_ADDR(0x7AE2EC) + _dyld_get_image_vmaddr_slide(0)) = static_cast<std::uint8_t>(!iToggle);
}

void CGame::GetScreenWidthAndHeight(int* w, int* h)
{
    RwVideoMode modeInfo{};
    if (RwEngineGetCurrentVideoMode && RwEngineGetVideoModeInfo)
    {
        const int mode = RwEngineGetCurrentVideoMode();
        if (mode >= 0 && RwEngineGetVideoModeInfo(&modeInfo, mode))
        {
            *w = modeInfo.width;
            *h = modeInfo.height;
            return;
        }
    }

    *w = RsGlobal ? RsGlobal->maximumWidth : 0;
    *h = RsGlobal ? RsGlobal->maximumHeight : 0;
}

void CGame::DisplayHUD(bool bDisp)
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0xEA300), "\xEA\x31\x00\x90") && CMemory::HasMappedInstruction(SA_ADDR(0xEA304), "\x4A\xA1\x0D\x91") && CMemory::HasMappedInstruction(SA_ADDR(0xEA308), "\x49\x01\x00\x39");
    if (!verified) {
        return;
    }

    *reinterpret_cast<std::uint8_t*>(SA_ADDR(0x726368) + _dyld_get_image_vmaddr_slide(0)) = bDisp;
    CGame::ToggleRadar(bDisp);
}

uint8_t CGame::FindFirstFreePlayerPedSlot()
{
    uint8_t x = 2;
	while (x != eMaxItem::PLAYER_PED_SLOTS)
	{
		if (!CPlayerPedSlot::IsActive(x)) {
			return x;
		}

		x++;
	}

    return 0;
}

CPlayerPed* CGame::NewPlayer(int iSkin, float fX, float fY, float fZ, float fRotation, [[maybe_unused]] uint8_t byteCreateMarker)
{
	// CNecessary::Log(OBFUSCATE("NewPlayer: %d %.2f %.2f %.2f %.2f %d"), iSkin, fX, fY, fZ, fRotation, byteCreateMarker);

	uint8_t bytePlayerNum = CGame::FindFirstFreePlayerPedSlot();
	if (!bytePlayerNum) {
		return nullptr;
	}

	CPlayerPed* pPlayerNew = new CPlayerPed(bytePlayerNum, iSkin, fX, fY, fZ, fRotation);

	return pPlayerNew;
}

void CGame::RemovePlayer(CPlayerPed* pPlayer)
{
	if (pPlayer)
	{
		CPlayerPedSlot::Set(pPlayer->m_bytePlayerNumber, 0);
		SAFE_DELETE(pPlayer);
	}
}

std::uint8_t CGame::GetActiveInterior()
{
    std::uint32_t dwRet = 0;
    CScripting::ScriptCommand(&get_active_interior, &dwRet);
    return static_cast<std::uint8_t>(dwRet);
}

bool CGame::IsDrawingBigMap()
{
    static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x1F19D4), "\xE8\x2C\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x1F19D8), "\x08\xA1\x37\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x1F19DC), "\x08\x31\x42\x39");
    if (!verified) {
        return true;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    return *reinterpret_cast<const std::uint8_t*>(SA_ADDR(0x790E74) + slide) != 0;
}

void CGame::SetPlayerStatsLocked(bool locked)
{
    const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x175340), "\x62\x87\x0C\x14") && CMemory::HasMappedInstruction(SA_ADDR(0x4970C8), "\x10\x0F\x00\x90", "__mg_gameproc") && CMemory::HasMappedInstruction(SA_ADDR(0x4970CC), "\x10\xE2\x49\x39", "__mg_gameproc") && CMemory::HasMappedInstruction(SA_ADDR(0x4970D4), "\xC0\x03\x5F\xD6", "__mg_gameproc");
    if (verified)
    {
        const auto slide = _dyld_get_image_vmaddr_slide(0);
        *reinterpret_cast<std::uint8_t*>(SA_ADDR(0x677278) + slide) = static_cast<std::uint8_t>(locked);
    }
}

void CGame::MenuSwitchOff()
{
    if (!CMemory::HasMappedInstruction(SA_ADDR(0x1EC924), "\xF4\x4F\xBE\xA9") || !CMemory::HasMappedInstruction(SA_ADDR(0x1E0324), "\x80\x31\x00\x94")) {
        return;
    }

    reinterpret_cast<void (*)()>(SA_ADDR(0x1EC924) + _dyld_get_image_vmaddr_slide(0))();
}

void CGame::RefreshStreamingAt(CVector pos) 
{
	CScripting::ScriptCommand(&refresh_streaming_at, pos.x, pos.y);
}

int CGame::GetPlayerMoney()
{
    if (!CWorld::Players) {
        return 0;
    }

    return *reinterpret_cast<const int*>(reinterpret_cast<std::uintptr_t>(CWorld::Players) + 0xF0);
}

void CGame::AddPlayerMoney(int money)
{
	CScripting::ScriptCommand(&add_to_player_money, 0, money);
}

void CGame::ResetPlayerMoney()
{
	int iMoney = CGame::GetPlayerMoney();
	if(!iMoney) {
		return;
	}

	CGame::AddPlayerMoney(iMoney < 0 ? abs(iMoney) : -(iMoney));
}

void CGame::SetPlayerMaxStats()
{
    const bool vehicle = CMemory::HasMappedInstruction(SA_ADDR(0x0AEC38), "\xE9\x23\xBE\x6D") && CMemory::HasMappedInstruction(SA_ADDR(0x0AEC54), "\xBB\x19\x03\x94");
    const bool weapon = CMemory::HasMappedInstruction(SA_ADDR(0x0AEB98), "\xE9\x23\xBE\x6D") && CMemory::HasMappedInstruction(SA_ADDR(0x0AEBB4), "\xE3\x19\x03\x94");
    if (!vehicle || !weapon) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    CGame::SetPlayerStatsLocked(false);
    reinterpret_cast<void (*)()>(SA_ADDR(0x0AEC38) + slide)(); // CCheat::VehicleSkillsCheat
    reinterpret_cast<void (*)()>(SA_ADDR(0x0AEB98) + slide)(); // CCheat::WeaponSkillsCheat
    CGame::SetPlayerStatsLocked(true);
}

bool CGame::IsAnimationLoaded(const char* szAnimFile)
{
    return CScripting::ScriptCommand(&is_animation_loaded, szAnimFile);
}

void CGame::RequestAnimation(const char* szAnimFile)
{
    CScripting::ScriptCommand(&request_animation, szAnimFile);
}

void CGame::LoadAnimationsAndModels()
{
    // CNecessary::Log(OBFUSCATE("CGame LoadAnimationsAndModels"));

    if (!CGame::IsAnimationLoaded(OBFUSCATE("PARACHUTE")))
    {
        CGame::RequestAnimation(OBFUSCATE("PARACHUTE"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("BAR")))
    {
        CGame::RequestAnimation(OBFUSCATE("BAR"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("GANGS")))
    {
        CGame::RequestAnimation(OBFUSCATE("GANGS"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("PAULNMAC")))
    {
        CGame::RequestAnimation(OBFUSCATE("PAULNMAC"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("VENDING")))
    {
        CGame::RequestAnimation(OBFUSCATE("VENDING"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("FIGHT_B")))
    {
        CGame::RequestAnimation(OBFUSCATE("FIGHT_B"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("FIGHT_C")))
    {
        CGame::RequestAnimation(OBFUSCATE("FIGHT_C"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("FIGHT_D")))
    {
        CGame::RequestAnimation(OBFUSCATE("FIGHT_D"));
    }

    if (!CGame::IsAnimationLoaded(OBFUSCATE("FIGHT_E")))
    {
        CGame::RequestAnimation(OBFUSCATE("FIGHT_E"));
    }

    if (!CStreaming::IsModelLoaded(3131))
    {
        CStreaming::RequestModel(3131);
    }

    if (!CStreaming::IsModelLoaded(1485))
    {
        CStreaming::RequestModel(1485);
    }

    if (!CStreaming::IsModelLoaded(1486))
    {
        CStreaming::RequestModel(1486);
    }

    if (!CStreaming::IsModelLoaded(1543))
    {
        CStreaming::RequestModel(1543);
    }

    if (!CStreaming::IsModelLoaded(1546))
    {
        CStreaming::RequestModel(1546);
    }

    if (!CStreaming::IsModelLoaded(344))
    {
        CStreaming::RequestModel(344);
    }

    if (!CStreaming::IsModelLoaded(343))
    {
        CStreaming::RequestModel(343);
    }

    if (!CStreaming::IsModelLoaded(342))
    {
        CStreaming::RequestModel(342);
    }

    if (!CStreaming::IsModelLoaded(330))
    {
        CStreaming::RequestModel(330);
    }
}

float CGame::FindGroundZForCoord(float x, float y, float z)
{
	float fGroundZ;
	CScripting::ScriptCommand(&get_ground_z, x, y, z, &fGroundZ);
	return fGroundZ;
}

void CGame::ToggleWidescreen(uint8_t Toggle)
{
	CScripting::ScriptCommand(&toggle_widescreen, Toggle);
}

void CGame::CreateExplosion(float X, float Y, float Z, int iType, float Radius)
{
	CScripting::ScriptCommand(&create_explosion_with_radius, X, Y, Z, iType, Radius);
}