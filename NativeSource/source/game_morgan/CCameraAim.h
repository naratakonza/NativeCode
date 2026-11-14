#pragma once

typedef struct _CAMERA_AIM
{
    float f1x, f1y, f1z;
    float pos1x, pos1y, pos1z;
    float pos2x, pos2y, pos2z;
    float f2x, f2y, f2z;
} CAMERA_AIM;
static_assert(sizeof(CAMERA_AIM) == 0x30);

extern float *m_fVerticalAngle;
extern float *m_fHorizontalAngle;
extern uint16_t *pbyteCameraMode;
extern uint16_t *wCameraMode2;

class CCameraAim
{
public:
    static void Initialise();
    static void GameResetLocalPlayerSkills();

    static CAMERA_AIM* GameGetInternalAim();

    static void GameSetLocalPlayerAim();
    static void GameStoreLocalPlayerAim();
    static void GameSetRemotePlayerAim(uint8_t bytePlayerID);
    static void GameStoreRemotePlayerAim(uint8_t bytePlayerID, CAMERA_AIM *pAim);

    static float GameGetAspectRatio();
    static float GameGetLocalPlayerCameraExtZoom();
    static void GameSetLocalPlayerCameraExtZoom();
    static void GameStoreLocalPlayerCameraExtZoom();
    static void GameSetRemotePlayerCameraExtZoom(uint8_t bytePlayerID);
    static void GameSetPlayerCameraExtZoom(uint8_t bytePlayerID, float fExtZoom, float fAspectRatio);

    static CAMERA_AIM* GameGetRemotePlayerAim(uint8_t bytePlayerNumber);
    static uint8_t GameGetLocalPlayerCameraMode();
    static void GameSetLocalPlayerCameraMode(uint8_t byteMode);
    static uint8_t GameGetPlayerCameraMode(uint8_t bytePlayerID);
    static void GameSetPlayerCameraMode(uint8_t byteMode, uint8_t bytePlayerNumber);

    static void GameUpdateLocalPlayerSkill(int iSkillType, uint16_t wSkillLevel);
    static void GameSetLocalPlayerSkills();
    static void GameStoreLocalPlayerSkills();
    static void GameSetRemotePlayerSkills(uint8_t bytePlayerNumber);
    static void GameStoreRemotePlayerSkills(uint8_t bytePlayerNumber, int iSkillType, uint16_t wSkillLevel);

};