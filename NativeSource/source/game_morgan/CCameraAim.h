#pragma once

#include <cstdint>

struct CAMERA_AIM
{
    float f1x, f1y, f1z;
    float pos1x, pos1y, pos1z;
    float pos2x, pos2y, pos2z;
    float f2x, f2y, f2z;
};

extern float* m_fVerticalAngle;
extern float* m_fHorizontalAngle;
extern std::uint16_t* pbyteCameraMode;
extern std::uint16_t* wCameraMode2;

class CCameraAim
{
public:
    static void Initialise();
    static void GameResetLocalPlayerSkills();

    static CAMERA_AIM* GameGetInternalAim();
    static void GameSetLocalPlayerAim();
    static void GameStoreLocalPlayerAim();
    static void GameSetRemotePlayerAim(std::uint8_t bytePlayerID);
    static void GameStoreRemotePlayerAim(std::uint8_t bytePlayerID, CAMERA_AIM* pAim);

    static float GameGetAspectRatio();
    static float GameGetLocalPlayerCameraExtZoom();
    static void GameSetLocalPlayerCameraExtZoom();
    static void GameStoreLocalPlayerCameraExtZoom();
    static void GameSetRemotePlayerCameraExtZoom(std::uint8_t bytePlayerID);
    static void GameSetPlayerCameraExtZoom(std::uint8_t bytePlayerID, float fExtZoom, float fAspectRatio);

    static CAMERA_AIM* GameGetRemotePlayerAim(std::uint8_t bytePlayerNumber);
    static std::uint8_t GameGetLocalPlayerCameraMode();
    static void GameSetLocalPlayerCameraMode(std::uint8_t byteMode);
    static std::uint8_t GameGetPlayerCameraMode(std::uint8_t bytePlayerID);
    static void GameSetPlayerCameraMode(std::uint8_t byteMode, std::uint8_t bytePlayerNumber);

    static void GameUpdateLocalPlayerSkill(int iSkillType, std::uint16_t wSkillLevel);
    static void GameSetLocalPlayerSkills();
    static void GameStoreLocalPlayerSkills();
    static void GameSetRemotePlayerSkills(std::uint8_t bytePlayerNumber);
    static void GameStoreRemotePlayerSkills(std::uint8_t bytePlayerNumber, int iSkillType, std::uint16_t wSkillLevel);

private:
    static std::uintptr_t GetActiveCam();
};
