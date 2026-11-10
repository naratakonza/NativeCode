#pragma once

#include "../vendor/rapidjson/fwd.h"

#include <cstdint>
#include <system_error>

#define MAX_SETTINGS_STRING 0x7F

struct stSettings
{
    char szNickName[24 + 1];
    char szIp[MAX_SETTINGS_STRING + 1];
    char szPassword[MAX_SETTINGS_STRING + 1];
    int port;
    int sampVersion;

    char szFont[40];
    char szCrosshair[50];
    float fFontSize;
    int iFontOutline;

    float fChatPosX;
    float fChatPosY;
    float fChatSizeX;
    float fChatSizeY;

    float fHealthBarWidth;
    float fHealthBarHeight;

    float fButtonEnterPassengerX;
    float fButtonEnterPassengerY;
    float fButtonEnterPassengerSize;

    float fButtonMicrophoneX;
    float fButtonMicrophoneY;
    float fButtonMicrophoneSize;

    float fButtonCameraCycleX;
    float fButtonCameraCycleY;
    float fButtonCameraCycleSize;

    float fJavaChatPosX;

    int last_server;
    bool bTimeStamp;

    int iShadow;
    int iSetAspectratio;
    int iSetButtonAlpha;
    int iSetSensitivity;
    int iSetZoomSensitivity;
    int iSetIncarSensitivity;
    int iSetFps;
    int iSetFov;
    int iRadarAlpha;
    int iRadarSectionMapAlpha;
    int iHudAlpha;
    std::uint32_t dwGPSColor;

    bool bGyroScope;
    int iGyroAimSensitivity;
    int iGyroOnfootSensitivity;
    int iGyroIncarSensitivity;
    int iGyroMode;
    int iLanguage;

    bool bJiggle;
    bool bMicStyle;
    bool bWeaponTag;
    bool bSwapWeapon;
    bool bVehicleInfoStyle;
    bool bHeadmove;
    bool bCameraToggleStyle;
    bool bCameraPlaneStyle;

    int iSetCrouchStyle;
    int iSetRadarTouchStyle;

    float fFogDistance;
    float fShadowDistance;
    float fRadarZoom;

    int iSetSoundVolume;
    int iSetMicroVolume;
    bool bVolumeSmooth;
    bool bHighPassFilter;

    bool bShowFps;
    bool bShowMem;
    bool bShowKeyF4;
    bool bShowKeyUPDOWN;
    bool bShowKeyLEFTRIGHT;
    bool bPCCamera;
};

class CSetting
{
public:
    static CSetting* pSetting;

    CSetting();

    void LoadSettings();
    void ResetToDefault();

    void WriteIntValue(const char* key, int value, bool writefile = true);
    void WriteBoolValue(const char* key, bool value, bool writefile = true);
    void WriteFloatValue(const char* key, float value, bool writefile = true);
    void WriteStringValue(const char* key, const char* value, bool writefile = true);

    const char* GetStringValue(const char* key);
    int GetIntValue(const char* key);
    bool GetBoolValue(const char* key);
    float GetFloatValue(const char* key);

    static CSetting* GetConfig() {
        return pSetting;
    }

private:
    stSettings m_Settings{};

    void InitDefault();
    void CreateDefault(rapidjson::Document& doc);
    bool WriteJsonToFile(const rapidjson::Document& doc);
    std::error_code ReadJsonFromFile(rapidjson::Document& doc);
    bool PrepareWrite(rapidjson::Document& doc, bool writefile);
    void ApplySettings(rapidjson::Document& doc);
    void ApplyClientSettings(const rapidjson::Value& clientSettings);
    void ApplyGuiSettings(const rapidjson::Value& gui);
    void ApplyGameSettings(const rapidjson::Value& game);
    void ApplyVoiceSettings(const rapidjson::Value& sound);
};