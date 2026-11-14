#include "../../Main.h"
#include "CCameraAim.h"
#include "Enums/eMaxItem.h"

#include <mach-o/dyld.h>

CAMERA_AIM* pcaInternalAim = nullptr;
float *m_fVerticalAngle = nullptr;
float *m_fHorizontalAngle = nullptr;
uint16_t *pbyteCameraMode = nullptr;
uint16_t *wCameraMode2 = nullptr;
float *pfCameraExtZoom = nullptr;
float *pfAspectRatio = nullptr;
float *pPlayerStats = nullptr;

CAMERA_AIM caLocalPlayerAim;
CAMERA_AIM caRemotePlayerAim[eMaxItem::PLAYER_PED_SLOTS];

uint8_t byteCameraMode[eMaxItem::PLAYER_PED_SLOTS];

float fCameraExtZoom[eMaxItem::PLAYER_PED_SLOTS];
float fLocalCameraExtZoom;

float fCameraAspectRatio[eMaxItem::PLAYER_PED_SLOTS];
float fLocalAspectRatio;

float fRemoteSkillLevel[eMaxItem::PLAYER_PED_SLOTS][11];
float fLocalSkillLevel[11];

void CCameraAim::Initialise()
{
    CNecessary::Log(OBFUSCATE("CCameraAim Initialise"));

    if (!CMemory::HasMappedInstruction(SA_ADDR(0x111390), "\xC8\x30\x00\xF0") || !CMemory::HasMappedInstruction(SA_ADDR(0x13538C), "\x08\x01\x29\x9B") || !CMemory::HasMappedInstruction(SA_ADDR(0x13A76C), "\x01\x18\x10\x79") || !CMemory::HasMappedInstruction(SA_ADDR(0x17574C), "\x29\x71\x3F\x91"))
    {
        CNecessary::Log(OBFUSCATE("CCameraAim GTA bindings unavailable"));
        return;
    }

    memset(&caLocalPlayerAim, 0, sizeof(CAMERA_AIM));
    memset(caRemotePlayerAim, 0, sizeof(CAMERA_AIM) * eMaxItem::PLAYER_PED_SLOTS);
    memset(byteCameraMode, 4, eMaxItem::PLAYER_PED_SLOTS);

    std::fill_n(fCameraAspectRatio, eMaxItem::PLAYER_PED_SLOTS, 0.333333f);
    std::fill_n(fCameraExtZoom, eMaxItem::PLAYER_PED_SLOTS, 1.0f);

    memset(fRemoteSkillLevel, 0, sizeof(fRemoteSkillLevel));
    memset(fLocalSkillLevel, 0, sizeof(fLocalSkillLevel));

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    pcaInternalAim = CCameraAim::GameGetInternalAim();
    m_fVerticalAngle = (float*)(SA_ADDR(0x72CF08 + 0x1FC) + slide);
    m_fHorizontalAngle = (float*)(SA_ADDR(0x72CF08 + 0x20C) + slide);
    pbyteCameraMode = (uint16_t*)(SA_ADDR(0x72CF08 + 0x186) + slide);
    wCameraMode2 = (uint16_t *)(SA_ADDR(0x72CF08 + 0x80C) + slide);
    pfAspectRatio = (float *)(SA_ADDR(0x80EB74) + slide);
    pfCameraExtZoom = (float*)(SA_ADDR(0x72CF08 + 0x204) + slide);
    pPlayerStats = (float *)(SA_ADDR(0x73AFDC) + slide);
}

void CCameraAim::GameResetLocalPlayerSkills()
{
    for (size_t i = 0; i < ARRAY_SIZE(fLocalSkillLevel); i++)
    {
        fLocalSkillLevel[i] = 999.0f;
    }

    CCameraAim::GameSetLocalPlayerSkills();
}

CAMERA_AIM* CCameraAim::GameGetInternalAim()
{
    return (CAMERA_AIM*)(SA_ADDR(0x72CF08 + 0x2E0) + _dyld_get_image_vmaddr_slide(0));
}

void CCameraAim::GameSetLocalPlayerAim()
{
    if (!pcaInternalAim) {
        return;
    }

    memcpy(pcaInternalAim, &caLocalPlayerAim, sizeof(CAMERA_AIM));
}

void CCameraAim::GameStoreLocalPlayerAim()
{
    if (!pcaInternalAim) {
        return;
    }

    memcpy(&caLocalPlayerAim, pcaInternalAim, sizeof(CAMERA_AIM));
}

CAMERA_AIM* CCameraAim::GameGetRemotePlayerAim(uint8_t bytePlayerNumber)
{
    return bytePlayerNumber < eMaxItem::PLAYER_PED_SLOTS ? &caRemotePlayerAim[bytePlayerNumber] : nullptr;
}

void CCameraAim::GameSetRemotePlayerAim(uint8_t bytePlayerID)
{
    if (!pcaInternalAim || bytePlayerID >= eMaxItem::PLAYER_PED_SLOTS) {
        return;
    }

    memcpy(pcaInternalAim, &caRemotePlayerAim[bytePlayerID], sizeof(CAMERA_AIM));
}

void CCameraAim::GameStoreRemotePlayerAim(uint8_t bytePlayerNumber, CAMERA_AIM* pAim)
{
    if (!pAim || bytePlayerNumber >= eMaxItem::PLAYER_PED_SLOTS || pAim == &caRemotePlayerAim[bytePlayerNumber]) {
        return;
    }

    memcpy(&caRemotePlayerAim[bytePlayerNumber], pAim, sizeof(CAMERA_AIM));
}

float CCameraAim::GameGetAspectRatio()
{
    return pfAspectRatio ? *pfAspectRatio : 0.0f;
}

float CCameraAim::GameGetLocalPlayerCameraExtZoom()
{
    return pfCameraExtZoom ? (*pfCameraExtZoom - 35.0f) * 0.028571429f : 0.0f;    // normalize for 35.0 to 70.0
}

void CCameraAim::GameSetLocalPlayerCameraExtZoom()
{
    if (!pfCameraExtZoom) {
        return;
    }

    *pfCameraExtZoom = fLocalCameraExtZoom;
    *pfAspectRatio = fLocalAspectRatio;
}

void CCameraAim::GameStoreLocalPlayerCameraExtZoom()
{
    if (!pfCameraExtZoom) {
        return;
    }

    fLocalCameraExtZoom = *pfCameraExtZoom;
    fLocalAspectRatio = *pfAspectRatio;
}

void CCameraAim::GameSetRemotePlayerCameraExtZoom(uint8_t bytePlayerID)
{
    if (!pfCameraExtZoom || bytePlayerID >= eMaxItem::PLAYER_PED_SLOTS) {
        return;
    }

    *pfCameraExtZoom = fCameraExtZoom[bytePlayerID] * 35.0f + 35.0f; // unnormalize for 35.0 to 70.0
    *pfAspectRatio = fCameraAspectRatio[bytePlayerID] + 1.0f;
}

void CCameraAim::GameSetPlayerCameraExtZoom(uint8_t bytePlayerID, float fExtZoom, float fAspectRatio)
{
    if (bytePlayerID >= eMaxItem::PLAYER_PED_SLOTS) {
        return;
    }

    fCameraExtZoom[bytePlayerID] = fExtZoom;
    fCameraAspectRatio[bytePlayerID] = fAspectRatio;
}

uint8_t CCameraAim::GameGetLocalPlayerCameraMode()
{
    return pbyteCameraMode ? static_cast<uint8_t>(*pbyteCameraMode) : 0;
}

void CCameraAim::GameSetLocalPlayerCameraMode(uint8_t byteMode)
{
    if (pbyteCameraMode)
    {
        *pbyteCameraMode = byteMode;
    }
}

uint8_t CCameraAim::GameGetPlayerCameraMode(uint8_t bytePlayerID)
{
    return bytePlayerID < eMaxItem::PLAYER_PED_SLOTS ? byteCameraMode[bytePlayerID] : 4;
}

void CCameraAim::GameSetPlayerCameraMode(uint8_t byteMode, uint8_t bytePlayerNumber)
{
    if (bytePlayerNumber >= eMaxItem::PLAYER_PED_SLOTS) {
        return;
    }

    byteCameraMode[bytePlayerNumber] = byteMode;
}

void CCameraAim::GameUpdateLocalPlayerSkill(int iSkillType, uint16_t wSkillLevel)
{
    if (!pPlayerStats) {
        return;
    }

    if (ARRAY_VALIDATE(fLocalSkillLevel, iSkillType))
    {
        pPlayerStats[69 + iSkillType] = (float)wSkillLevel;
    }
}

void CCameraAim::GameSetLocalPlayerSkills()
{
    if (!pPlayerStats) {
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(fLocalSkillLevel); i++)
    {
        pPlayerStats[69 + i] = fLocalSkillLevel[i];
    }
}

void CCameraAim::GameStoreLocalPlayerSkills()
{
    if (!pPlayerStats) {
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(fLocalSkillLevel); i++)
    {
        fLocalSkillLevel[i] = pPlayerStats[69 + i];
    }
}

void CCameraAim::GameSetRemotePlayerSkills(uint8_t bytePlayerNumber)
{
    if (!pPlayerStats || bytePlayerNumber >= eMaxItem::PLAYER_PED_SLOTS) {
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(fLocalSkillLevel); i++)
    {
        pPlayerStats[69 + i] = fRemoteSkillLevel[bytePlayerNumber][i];
    }
}

void CCameraAim::GameStoreRemotePlayerSkills(uint8_t bytePlayerNumber, int iSkillType, uint16_t wSkillLevel)
{
    if (bytePlayerNumber >= eMaxItem::PLAYER_PED_SLOTS) {
        return;
    }

    if (ARRAY_VALIDATE(fLocalSkillLevel, iSkillType))
    {
        fRemoteSkillLevel[bytePlayerNumber][iSkillType] = (float)wSkillLevel;
    }
}