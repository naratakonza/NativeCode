#include "../../Main.h"
#include "CCameraAim.h"
#include "CCamera.h"
#include "Enums/eMaxItem.h"

#include <mach-o/dyld.h>

static_assert(sizeof(CAMERA_AIM) == 0x30);
static_assert(offsetof(CAMERA_AIM, pos1x) == 0x0C);
static_assert(offsetof(CAMERA_AIM, pos2x) == 0x18);
static_assert(offsetof(CAMERA_AIM, f2x) == 0x24);

float* m_fVerticalAngle = nullptr;
float* m_fHorizontalAngle = nullptr;
std::uint16_t* pbyteCameraMode = nullptr;
std::uint16_t* wCameraMode2 = nullptr;

static float* pfAspectRatio = nullptr;
static float* pPlayerStats = nullptr;
static CAMERA_AIM caLocalPlayerAim;
static CAMERA_AIM caRemotePlayerAim[eMaxItem::PLAYER_PED_SLOTS];
static std::uint8_t byteCameraMode[eMaxItem::PLAYER_PED_SLOTS];
static float fCameraExtZoom[eMaxItem::PLAYER_PED_SLOTS];
static float fLocalCameraExtZoom;
static float fCameraAspectRatio[eMaxItem::PLAYER_PED_SLOTS];
static float fLocalAspectRatio;
static constexpr int firstWeaponSkill = 69;
static constexpr int weaponSkillCount = 11;
static float fRemoteSkillLevel[eMaxItem::PLAYER_PED_SLOTS][weaponSkillCount];
static float fLocalSkillLevel[weaponSkillCount];

void CCameraAim::Initialise()
{
    CNecessary::Log(OBFUSCATE("CCameraAim Initialise"));

    if (!CMemory::HasMappedInstruction(SA_ADDR(0x111390), "\xC8\x30\x00\xF0") || !CMemory::HasMappedInstruction(SA_ADDR(0x13538C), "\x08\x01\x29\x9B") || !CMemory::HasMappedInstruction(SA_ADDR(0x13A76C), "\x01\x18\x10\x79") || !CMemory::HasMappedInstruction(SA_ADDR(0x17574C), "\x29\x71\x3F\x91"))
    {
        CNecessary::Log(OBFUSCATE("CCameraAim GTA bindings unavailable"));
        return;
    }

    caLocalPlayerAim = {};
    std::memset(caRemotePlayerAim, 0, sizeof(caRemotePlayerAim));
    std::fill_n(byteCameraMode, eMaxItem::PLAYER_PED_SLOTS, 4);
    std::fill_n(fCameraAspectRatio, eMaxItem::PLAYER_PED_SLOTS, 1.0f / 3.0f);
    std::fill_n(fCameraExtZoom, eMaxItem::PLAYER_PED_SLOTS, 1.0f);
    std::memset(fRemoteSkillLevel, 0, sizeof(fRemoteSkillLevel));
    std::memset(fLocalSkillLevel, 0, sizeof(fLocalSkillLevel));

    const auto camera = reinterpret_cast<std::uintptr_t>(&CCamera::TheCamera);
    const auto slide = _dyld_get_image_vmaddr_slide(0);
    m_fVerticalAngle = reinterpret_cast<float*>(camera + 0x1FC);
    m_fHorizontalAngle = reinterpret_cast<float*>(camera + 0x20C);
    pbyteCameraMode = reinterpret_cast<std::uint16_t*>(camera + 0x186);
    wCameraMode2 = reinterpret_cast<std::uint16_t*>(camera + 0x80C);
    pfAspectRatio = reinterpret_cast<float*>(SA_ADDR(0x80EB74) + slide);
    pPlayerStats = reinterpret_cast<float*>(SA_ADDR(0x73AFDC) + slide);

    CCameraAim::GameStoreLocalPlayerAim();
    CCameraAim::GameStoreLocalPlayerCameraExtZoom();
    CCameraAim::GameStoreLocalPlayerSkills();
}

std::uintptr_t CCameraAim::GetActiveCam()
{
    if (!wCameraMode2) {
        return 0;
    }

    const auto camera = reinterpret_cast<std::uintptr_t>(&CCamera::TheCamera);
    const auto active = *reinterpret_cast<const std::uint8_t*>(camera + 0x5F);
    if (active >= 3) {
        return 0;
    }

    return camera + 0x228 * active;
}

void CCameraAim::GameResetLocalPlayerSkills()
{
    std::fill_n(fLocalSkillLevel, weaponSkillCount, 999.0f);
    CCameraAim::GameSetLocalPlayerSkills();
}

CAMERA_AIM* CCameraAim::GameGetInternalAim()
{
    const auto camera = CCameraAim::GetActiveCam();
    return camera ? reinterpret_cast<CAMERA_AIM*>(camera + 0x2E0) : nullptr;
}

void CCameraAim::GameSetLocalPlayerAim()
{
    if (auto* pAim = CCameraAim::GameGetInternalAim())
    {
        std::memcpy(pAim, &caLocalPlayerAim, sizeof(CAMERA_AIM));
    }
}

void CCameraAim::GameStoreLocalPlayerAim()
{
    if (const auto* pAim = CCameraAim::GameGetInternalAim())
    {
        std::memcpy(&caLocalPlayerAim, pAim, sizeof(CAMERA_AIM));
    }
}

CAMERA_AIM* CCameraAim::GameGetRemotePlayerAim(std::uint8_t bytePlayerNumber)
{
    return bytePlayerNumber < eMaxItem::PLAYER_PED_SLOTS ? &caRemotePlayerAim[bytePlayerNumber] : nullptr;
}

void CCameraAim::GameSetRemotePlayerAim(std::uint8_t bytePlayerID)
{
    auto* pAim = CCameraAim::GameGetInternalAim();
    const auto* pRemoteAim = CCameraAim::GameGetRemotePlayerAim(bytePlayerID);
    if (pAim && pRemoteAim)
    {
        std::memcpy(pAim, pRemoteAim, sizeof(CAMERA_AIM));
    }
}

void CCameraAim::GameStoreRemotePlayerAim(std::uint8_t bytePlayerNumber, CAMERA_AIM* pAim)
{
    auto* pRemoteAim = CCameraAim::GameGetRemotePlayerAim(bytePlayerNumber);
    if (pAim && pRemoteAim && pAim != pRemoteAim)
    {
        std::memcpy(pRemoteAim, pAim, sizeof(CAMERA_AIM));
    }
}

float CCameraAim::GameGetAspectRatio()
{
    return pfAspectRatio ? *pfAspectRatio : 0.0f;
}

float CCameraAim::GameGetLocalPlayerCameraExtZoom()
{
    const auto camera = CCameraAim::GetActiveCam();
    return camera ? (*reinterpret_cast<const float*>(camera + 0x204) - 35.0f) / 35.0f : 0.0f;
}

void CCameraAim::GameSetLocalPlayerCameraExtZoom()
{
    if (const auto camera = CCameraAim::GetActiveCam())
    {
        *reinterpret_cast<float*>(camera + 0x204) = fLocalCameraExtZoom;
        *pfAspectRatio = fLocalAspectRatio;
    }
}

void CCameraAim::GameStoreLocalPlayerCameraExtZoom()
{
    if (const auto camera = CCameraAim::GetActiveCam())
    {
        fLocalCameraExtZoom = *reinterpret_cast<const float*>(camera + 0x204);
        fLocalAspectRatio = *pfAspectRatio;
    }
}

void CCameraAim::GameSetRemotePlayerCameraExtZoom(std::uint8_t bytePlayerID)
{
    const auto camera = CCameraAim::GetActiveCam();
    if (camera && bytePlayerID < eMaxItem::PLAYER_PED_SLOTS)
    {
        *reinterpret_cast<float*>(camera + 0x204) = fCameraExtZoom[bytePlayerID] * 35.0f + 35.0f;
        *pfAspectRatio = fCameraAspectRatio[bytePlayerID] + 1.0f;
    }
}

void CCameraAim::GameSetPlayerCameraExtZoom(std::uint8_t bytePlayerID, float fExtZoom, float fAspectRatio)
{
    if (bytePlayerID < eMaxItem::PLAYER_PED_SLOTS)
    {
        fCameraExtZoom[bytePlayerID] = fExtZoom;
        fCameraAspectRatio[bytePlayerID] = fAspectRatio;
    }
}

std::uint8_t CCameraAim::GameGetLocalPlayerCameraMode()
{
    const auto camera = CCameraAim::GetActiveCam();
    return camera ? static_cast<std::uint8_t>(*reinterpret_cast<const std::uint16_t*>(camera + 0x186)) : 0;
}

void CCameraAim::GameSetLocalPlayerCameraMode(std::uint8_t byteMode)
{
    if (const auto camera = CCameraAim::GetActiveCam())
    {
        *reinterpret_cast<std::uint16_t*>(camera + 0x186) = byteMode;
    }
}

std::uint8_t CCameraAim::GameGetPlayerCameraMode(std::uint8_t bytePlayerID)
{
    return bytePlayerID < eMaxItem::PLAYER_PED_SLOTS ? byteCameraMode[bytePlayerID] : 4;
}

void CCameraAim::GameSetPlayerCameraMode(std::uint8_t byteMode, std::uint8_t bytePlayerNumber)
{
    if (bytePlayerNumber < eMaxItem::PLAYER_PED_SLOTS)
    {
        byteCameraMode[bytePlayerNumber] = byteMode;
    }
}

void CCameraAim::GameUpdateLocalPlayerSkill(int iSkillType, std::uint16_t wSkillLevel)
{
    if (pPlayerStats && iSkillType >= 0 && iSkillType < weaponSkillCount)
    {
        pPlayerStats[firstWeaponSkill + iSkillType] = static_cast<float>(wSkillLevel);
    }
}

void CCameraAim::GameSetLocalPlayerSkills()
{
    if (pPlayerStats)
    {
        std::copy_n(fLocalSkillLevel, weaponSkillCount, pPlayerStats + firstWeaponSkill);
    }
}

void CCameraAim::GameStoreLocalPlayerSkills()
{
    if (pPlayerStats)
    {
        std::copy_n(pPlayerStats + firstWeaponSkill, weaponSkillCount, fLocalSkillLevel);
    }
}

void CCameraAim::GameSetRemotePlayerSkills(std::uint8_t bytePlayerNumber)
{
    if (pPlayerStats && bytePlayerNumber < eMaxItem::PLAYER_PED_SLOTS)
    {
        std::copy_n(fRemoteSkillLevel[bytePlayerNumber], weaponSkillCount, pPlayerStats + firstWeaponSkill);
    }
}

void CCameraAim::GameStoreRemotePlayerSkills(std::uint8_t bytePlayerNumber, int iSkillType, std::uint16_t wSkillLevel)
{
    if (bytePlayerNumber < eMaxItem::PLAYER_PED_SLOTS && iSkillType >= 0 && iSkillType < weaponSkillCount)
    {
        fRemoteSkillLevel[bytePlayerNumber][iSkillType] = static_cast<float>(wSkillLevel);
    }
}
