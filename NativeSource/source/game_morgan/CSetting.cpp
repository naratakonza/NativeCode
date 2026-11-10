#include "../../Main.h"
#include "Enums/eLanguage.h"
#include "../utils/Storage/CStorage.h"
#include "../utils/Storage/CStorageSettings.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

using rapidjson::Document;
using rapidjson::StringRef;
using rapidjson::Value;
using rapidjson::kObjectType;

template<std::size_t Size>
static void CopySettingString(char (&buffer)[Size], const char* value)
{
    const std::string_view text(value);
    auto length = std::min(text.find_first_of("\r\n"), text.size());
    if (length >= Size)
    {
        length = Size - 1;
        while (length && (static_cast<unsigned char>(text[length]) & 0xC0) == 0x80)
        {
            --length;
        }
    }
    std::memmove(buffer, value, length);
    buffer[length] = '\0';
}

static bool MergeSettingsDefaults(Value& object, const Value& defaults, Document::AllocatorType& allocator)
{
    bool modified = false;
    for (const auto& item : defaults.GetObject())
    {
        auto member = object.FindMember(item.name);
        if (member == object.MemberEnd())
        {
            object.AddMember(Value(item.name, allocator), Value(item.value, allocator), allocator);
            modified = true;
        }
        else if (item.value.IsObject() && member->value.IsObject())
        {
            modified |= MergeSettingsDefaults(member->value, item.value, allocator);
        }
        else if (member->value.GetType() != item.value.GetType() &&
                 !(member->value.IsBool() && item.value.IsBool()))
        {
            member->value.CopyFrom(item.value, allocator);
            modified = true;
        }
    }
    return modified;
}

CSetting* CSetting::pSetting = nullptr;

CSetting::CSetting()
{
    CSetting::LoadSettings();
}

void CSetting::LoadSettings()
{
    CSetting::InitDefault();

    Document doc;
    const auto error = CSetting::ReadJsonFromFile(doc);
    if (error)
    {
        if (error == std::errc::no_such_file_or_directory)
        {
            CSetting::CreateDefault(doc);
            if (CSetting::WriteJsonToFile(doc)) CNecessary::Log(OBFUSCATE("Created SAMP/settings.json"));
        }
        else
        {
            CNecessary::Log(OBFUSCATE("SAMP settings unavailable (error %d), using defaults"), error.value());
        }
        return;
    }

    Document defaults;
    CSetting::CreateDefault(defaults);
    const bool modified = MergeSettingsDefaults(doc, defaults, doc.GetAllocator());
    CSetting::ApplySettings(doc);
    if (modified) CSetting::WriteJsonToFile(doc);
    CNecessary::Log(OBFUSCATE("Loaded SAMP/settings.json"));
}

void CSetting::ResetToDefault()
{
    CSetting::InitDefault();
    Document doc;
    CSetting::CreateDefault(doc);
    CSetting::WriteJsonToFile(doc);
}

void CSetting::InitDefault()
{
    memset(&m_Settings, 0, sizeof(stSettings));
    std::snprintf(m_Settings.szNickName, sizeof(m_Settings.szNickName), OBFUSCATE("Morgan_%d%d"), std::rand() % 1000, std::rand() % 1000);
    CopySettingString(m_Settings.szIp, OBFUSCATE("127.0.0.1"));
    CopySettingString(m_Settings.szPassword, OBFUSCATE(""));
    CopySettingString(m_Settings.szFont, OBFUSCATE("Arial.ttf"));
    CopySettingString(m_Settings.szCrosshair, OBFUSCATE("default"));

    CSetting::m_Settings.port = 7777;
    CSetting::m_Settings.sampVersion = 0;
    CSetting::m_Settings.iLanguage = eLanguage::LANGUAGE_TH;
    CSetting::m_Settings.fFontSize = 30.0f;
    CSetting::m_Settings.iFontOutline = 2;
    CSetting::m_Settings.fChatPosX = 300.0f;
    CSetting::m_Settings.fChatPosY = 56.0f;
    CSetting::m_Settings.fChatSizeX = 1150.0f;
    CSetting::m_Settings.fChatSizeY = 220.0f;
    CSetting::m_Settings.fHealthBarWidth = 60.0f;
    CSetting::m_Settings.fHealthBarHeight = 10.0f;
    CSetting::m_Settings.fButtonEnterPassengerSize = 108.0f;
    CSetting::m_Settings.fButtonEnterPassengerX = 150.0f;
    CSetting::m_Settings.fButtonEnterPassengerY = 394.0f;
    CSetting::m_Settings.fButtonMicrophoneSize = 26.0f;
    CSetting::m_Settings.fButtonMicrophoneX = 1310.0f;
    CSetting::m_Settings.fButtonMicrophoneY = 506.0f;
    CSetting::m_Settings.fButtonCameraCycleX = 1770.0f;
    CSetting::m_Settings.fButtonCameraCycleY = 298.0f;
    CSetting::m_Settings.fButtonCameraCycleSize = 108.0f;
    CSetting::m_Settings.bTimeStamp = true;
    CSetting::m_Settings.iShadow = 1;
    CSetting::m_Settings.iSetAspectratio = 0;
    CSetting::m_Settings.iSetButtonAlpha = 255;
    CSetting::m_Settings.iSetSensitivity = 50;
    CSetting::m_Settings.iSetZoomSensitivity = 150;
    CSetting::m_Settings.iSetIncarSensitivity = 150;
    CSetting::m_Settings.iSetFps = 0;
    CSetting::m_Settings.iSetFov = 77;
    CSetting::m_Settings.iRadarAlpha = 255;
    CSetting::m_Settings.iRadarSectionMapAlpha = 255;
    CSetting::m_Settings.iHudAlpha = 255;
    CSetting::m_Settings.dwGPSColor = 0xFF00C8FF;
    CSetting::m_Settings.bGyroScope = false;
    CSetting::m_Settings.iGyroAimSensitivity = 170;
    CSetting::m_Settings.iGyroOnfootSensitivity = 170;
    CSetting::m_Settings.iGyroIncarSensitivity = 170;
    CSetting::m_Settings.iGyroMode = 2; // GYRO_ALL_MODE
    CSetting::m_Settings.bJiggle = true;
    CSetting::m_Settings.bMicStyle = true;
    CSetting::m_Settings.bWeaponTag = false;
    CSetting::m_Settings.bSwapWeapon = true;
    CSetting::m_Settings.bVehicleInfoStyle = false;
    CSetting::m_Settings.bHeadmove = true;
    CSetting::m_Settings.bCameraToggleStyle = false;
    CSetting::m_Settings.bCameraPlaneStyle = true;
    CSetting::m_Settings.iSetCrouchStyle = 0;
    CSetting::m_Settings.iSetRadarTouchStyle = 0;
    CSetting::m_Settings.last_server = -1;

    CSetting::m_Settings.fJavaChatPosX = 16.0f;
    CSetting::m_Settings.fFogDistance = 300.0f;
    CSetting::m_Settings.fShadowDistance = 15.0f;
    CSetting::m_Settings.fRadarZoom = 360.0f;

    CSetting::m_Settings.iSetSoundVolume = 100;
    CSetting::m_Settings.iSetMicroVolume = 100;
    CSetting::m_Settings.bVolumeSmooth = false;
    CSetting::m_Settings.bHighPassFilter = false;

    CSetting::m_Settings.bShowFps = true;
    CSetting::m_Settings.bShowMem = true;
    CSetting::m_Settings.bShowKeyF4 = false;
    CSetting::m_Settings.bShowKeyUPDOWN = true;
    CSetting::m_Settings.bShowKeyLEFTRIGHT = true;
    CSetting::m_Settings.bPCCamera = false;

    CNecessary::Log(OBFUSCATE("Settings Loaded With Defaults"));
}

void CSetting::CreateDefault(Document& doc)
{
    doc.SetObject();
    Document::AllocatorType& allocator = doc.GetAllocator();

    Value client(kObjectType);
    Value clientSettings(kObjectType);
    clientSettings.AddMember("PlayerName", StringRef(m_Settings.szNickName), allocator);
    clientSettings.AddMember("ServerPassword", "", allocator);
    clientSettings.AddMember("ServerIp", StringRef(m_Settings.szIp), allocator);
    clientSettings.AddMember("ServerPort", 7777, allocator);
    clientSettings.AddMember("LastServer", -1, allocator);
    clientSettings.AddMember("sampversion", 0, allocator);
    client.AddMember("settings", clientSettings, allocator);
    doc.AddMember("client", client, allocator);

    Value gui(kObjectType);
    gui.AddMember("Font", StringRef(m_Settings.szFont), allocator);
    gui.AddMember("Crosshair", StringRef(m_Settings.szCrosshair), allocator);
    gui.AddMember("FontSize", 30.0f, allocator);
    gui.AddMember("FontOutline", 2, allocator);
    gui.AddMember("ChatPosX", 300.0f, allocator);
    gui.AddMember("ChatPosY", 56.0f, allocator);
    gui.AddMember("ChatSizeX", 1150.0f, allocator);
    gui.AddMember("ChatSizeY", 220.0f, allocator);
    gui.AddMember("HealthBarWidth", 60.0f, allocator);
    gui.AddMember("HealthBarHeight", 10.0f, allocator);
    gui.AddMember("MicrophoneSize", 26.0f, allocator);
    gui.AddMember("MicrophoneX", 1310.0f, allocator);
    gui.AddMember("MicrophoneY", 506.0f, allocator);
    gui.AddMember("EnterPassengerSize", 108.0f, allocator);
    gui.AddMember("EnterPassengerX", 150.0f, allocator);
    gui.AddMember("EnterPassengerY", 394.0f, allocator);
    gui.AddMember("CameraCycleSize", 108.0f, allocator);
    gui.AddMember("CameraCycleX", 1770.0f, allocator);
    gui.AddMember("CameraCycleY", 298.0f, allocator);
    gui.AddMember("timestamp", true, allocator);
    gui.AddMember("JavaChatPosX", 16.0f, allocator);
    gui.AddMember("showfps", true, allocator);
    gui.AddMember("showmem", true, allocator);
    gui.AddMember("keyf4", false, allocator);
    gui.AddMember("keyupdown", true, allocator);
    gui.AddMember("keyleftright", true, allocator);
    gui.AddMember("language", static_cast<int>(eLanguage::LANGUAGE_TH), allocator);
    doc.AddMember("gui", gui, allocator);

    Value game(kObjectType);
    game.AddMember("shadow", 1, allocator);
    game.AddMember("visualeffects", 1, allocator);
    game.AddMember("setaspectratio", 0, allocator);
    game.AddMember("setbuttonalpha", 255, allocator);
    game.AddMember("setsensitivity", 50, allocator);
    game.AddMember("setzoomsensitivity", 150, allocator);
    game.AddMember("setincarsensitivity", 150, allocator);
    game.AddMember("setfps", 0, allocator);
    game.AddMember("setfov", 77, allocator);
    game.AddMember("radaralpha", 255, allocator);
    game.AddMember("radarsectionmapalpha", 255, allocator);
    game.AddMember("hudalpha", 255, allocator);
    game.AddMember("gpscolor", 0xFF00C8FF, allocator);
    game.AddMember("gyroscope", false, allocator);
    game.AddMember("gyroaimsens", 170, allocator);
    game.AddMember("gyroonfootsens", 170, allocator);
    game.AddMember("gyroincarsens", 170, allocator);
    game.AddMember("gyromode", 2, allocator);
    game.AddMember("setjiggle", true, allocator);
    game.AddMember("setmicstyle", true, allocator);
    game.AddMember("weapontag", false, allocator);
    game.AddMember("fogdistance", 300.0f, allocator);
    game.AddMember("shadowdistance", 15.0f, allocator);
    game.AddMember("radarzoom", 360.0f, allocator);
    game.AddMember("setweaponswitch", true, allocator);
    game.AddMember("setvehinfostyle", false, allocator);
    game.AddMember("setcrouchstyle", 0, allocator);
    game.AddMember("setradartouchstyle", 0, allocator);
    game.AddMember("setheadmove", true, allocator);
    game.AddMember("cameratoggle", false, allocator);
    game.AddMember("cameraplanestyle", true, allocator);
    game.AddMember("pccamera", false, allocator);
    doc.AddMember("game", game, allocator);

    Value sound(kObjectType);
    sound.AddMember("setvolume", 100, allocator);
    sound.AddMember("setvolumesmooth", false, allocator);
    sound.AddMember("sethighpassfilter", false, allocator);
    sound.AddMember("setsoundvolume", 100, allocator);
    doc.AddMember("sound", sound, allocator);
}

void CSetting::ApplyClientSettings(const Value& clientSettings)
{
    if (clientSettings.HasMember(OBFUSCATE("PlayerName")) && clientSettings["PlayerName"].IsString())
    {
        CopySettingString(CSetting::m_Settings.szNickName, clientSettings["PlayerName"].GetString());
    }

    if (clientSettings.HasMember(OBFUSCATE("ServerPassword")) && clientSettings["ServerPassword"].IsString())
    {
        CopySettingString(CSetting::m_Settings.szPassword, clientSettings["ServerPassword"].GetString());
    }

    if (clientSettings.HasMember(OBFUSCATE("ServerIp")) && clientSettings["ServerIp"].IsString())
    {
        CopySettingString(CSetting::m_Settings.szIp, clientSettings["ServerIp"].GetString());
    }

    if (clientSettings.HasMember(OBFUSCATE("ServerPort")) && clientSettings["ServerPort"].IsInt())
    {
        CSetting::m_Settings.port = clientSettings["ServerPort"].GetInt();
    }

    if (clientSettings.HasMember(OBFUSCATE("LastServer")) && clientSettings["LastServer"].IsInt())
    {
        CSetting::m_Settings.last_server = clientSettings["LastServer"].GetInt();
    }

    if (clientSettings.HasMember(OBFUSCATE("sampversion")) && clientSettings["sampversion"].IsInt())
    {
        CSetting::m_Settings.sampVersion = clientSettings["sampversion"].GetInt();
    }
}

void CSetting::ApplyGuiSettings(const Value& gui)
{
    if (gui.HasMember(OBFUSCATE("Font")) && gui["Font"].IsString())
    {
        CopySettingString(m_Settings.szFont, gui["Font"].GetString());
    }

    if (gui.HasMember(OBFUSCATE("Crosshair")) && gui["Crosshair"].IsString())
    {
        CopySettingString(m_Settings.szCrosshair, gui["Crosshair"].GetString());
    }

    if (gui.HasMember(OBFUSCATE("FontSize")) && gui["FontSize"].IsFloat())
    {
        CSetting::m_Settings.fFontSize = gui["FontSize"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("FontOutline")) && gui["FontOutline"].IsInt())
    {
        CSetting::m_Settings.iFontOutline = gui["FontOutline"].GetInt();
    }

    if (gui.HasMember(OBFUSCATE("ChatPosX")) && gui["ChatPosX"].IsFloat())
    {
        CSetting::m_Settings.fChatPosX = gui["ChatPosX"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("ChatPosY")) && gui["ChatPosY"].IsFloat())
    {
        CSetting::m_Settings.fChatPosY = gui["ChatPosY"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("ChatSizeX")) && gui["ChatSizeX"].IsFloat())
    {
        CSetting::m_Settings.fChatSizeX = gui["ChatSizeX"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("ChatSizeY")) && gui["ChatSizeY"].IsFloat())
    {
        CSetting::m_Settings.fChatSizeY = gui["ChatSizeY"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("HealthBarWidth")) && gui["HealthBarWidth"].IsFloat())
    {
        CSetting::m_Settings.fHealthBarWidth = gui["HealthBarWidth"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("HealthBarHeight")) && gui["HealthBarHeight"].IsFloat())
    {
        CSetting::m_Settings.fHealthBarHeight = gui["HealthBarHeight"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("EnterPassengerSize")) && gui["EnterPassengerSize"].IsFloat())
    {
        CSetting::m_Settings.fButtonEnterPassengerSize = gui["EnterPassengerSize"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("EnterPassengerX")) && gui["EnterPassengerX"].IsFloat())
    {
        CSetting::m_Settings.fButtonEnterPassengerX = gui["EnterPassengerX"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("EnterPassengerY")) && gui["EnterPassengerY"].IsFloat())
    {
        CSetting::m_Settings.fButtonEnterPassengerY = gui["EnterPassengerY"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("MicrophoneSize")))
    {
        const auto& value = gui["MicrophoneSize"];
        if (value.IsInt())
        {
            CSetting::m_Settings.fButtonMicrophoneSize = value.GetInt();
        }
        else if (value.IsFloat())
        {
            CSetting::m_Settings.fButtonMicrophoneSize = static_cast<int>(value.GetFloat());
        }
    }

    if (gui.HasMember(OBFUSCATE("MicrophoneX")) && gui["MicrophoneX"].IsFloat())
    {
        CSetting::m_Settings.fButtonMicrophoneX = gui["MicrophoneX"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("MicrophoneY")) && gui["MicrophoneY"].IsFloat())
    {
        CSetting::m_Settings.fButtonMicrophoneY = gui["MicrophoneY"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("CameraCycleX")) && gui["CameraCycleX"].IsFloat())
    {
        CSetting::m_Settings.fButtonCameraCycleX = gui["CameraCycleX"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("CameraCycleY")) && gui["CameraCycleY"].IsFloat())
    {
        CSetting::m_Settings.fButtonCameraCycleY = gui["CameraCycleY"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("CameraCycleSize")) && gui["CameraCycleSize"].IsFloat())
    {
        CSetting::m_Settings.fButtonCameraCycleSize = gui["CameraCycleSize"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("timestamp")) && (gui["timestamp"].IsBool() || gui["timestamp"].IsInt()))
    {
        CSetting::m_Settings.bTimeStamp = gui["timestamp"].IsBool() ? gui["timestamp"].GetBool() : gui["timestamp"].GetInt();
    }

    if (gui.HasMember(OBFUSCATE("JavaChatPosX")) && gui["JavaChatPosX"].IsFloat())
    {
        CSetting::m_Settings.fJavaChatPosX = gui["JavaChatPosX"].GetFloat();
    }

    if (gui.HasMember(OBFUSCATE("showfps")) && (gui["showfps"].IsBool() || gui["showfps"].IsInt()))
    {
        CSetting::m_Settings.bShowFps = gui["showfps"].IsBool() ? gui["showfps"].GetBool() : gui["showfps"].GetInt();
    }

    if (gui.HasMember(OBFUSCATE("showmem")) && (gui["showmem"].IsBool() || gui["showmem"].IsInt()))
    {
        CSetting::m_Settings.bShowMem = gui["showmem"].IsBool() ? gui["showmem"].GetBool() : gui["showmem"].GetInt();
    }

    if (gui.HasMember(OBFUSCATE("keyf4")) && (gui["keyf4"].IsBool() || gui["keyf4"].IsInt()))
    {
        CSetting::m_Settings.bShowKeyF4 = gui["keyf4"].IsBool() ? gui["keyf4"].GetBool() : gui["keyf4"].GetInt();
    }

    if (gui.HasMember(OBFUSCATE("keyupdown")) && (gui["keyupdown"].IsBool() || gui["keyupdown"].IsInt()))
    {
        CSetting::m_Settings.bShowKeyUPDOWN = gui["keyupdown"].IsBool() ? gui["keyupdown"].GetBool() : gui["keyupdown"].GetInt();
    }

    if (gui.HasMember(OBFUSCATE("keyleftright")) && (gui["keyleftright"].IsBool() || gui["keyleftright"].IsInt()))
    {
        CSetting::m_Settings.bShowKeyLEFTRIGHT = gui["keyleftright"].IsBool() ? gui["keyleftright"].GetBool() : gui["keyleftright"].GetInt();
    }

    if (gui.HasMember(OBFUSCATE("language")) && gui["language"].IsInt())
    {
        CSetting::m_Settings.iLanguage = gui["language"].GetInt();
    }
}

void CSetting::ApplyGameSettings(const Value& game)
{
    if (game.HasMember(OBFUSCATE("shadow")) && game["shadow"].IsInt())
    {
        CSetting::m_Settings.iShadow = game["shadow"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setaspectratio")) && game["setaspectratio"].IsInt())
    {
        CSetting::m_Settings.iSetAspectratio = game["setaspectratio"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setbuttonalpha")) && game["setbuttonalpha"].IsInt())
    {
        CSetting::m_Settings.iSetButtonAlpha = game["setbuttonalpha"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setsensitivity")) && game["setsensitivity"].IsInt())
    {
        CSetting::m_Settings.iSetSensitivity = game["setsensitivity"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setzoomsensitivity")) && game["setzoomsensitivity"].IsInt())
    {
        CSetting::m_Settings.iSetZoomSensitivity = game["setzoomsensitivity"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setincarsensitivity")) && game["setincarsensitivity"].IsInt())
    {
        CSetting::m_Settings.iSetIncarSensitivity = game["setincarsensitivity"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setfps")) && game["setfps"].IsInt())
    {
        CSetting::m_Settings.iSetFps = game["setfps"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setfov")) && game["setfov"].IsInt())
    {
        CSetting::m_Settings.iSetFov = game["setfov"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("radaralpha")) && game["radaralpha"].IsInt())
    {
        CSetting::m_Settings.iRadarAlpha = game["radaralpha"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("radarsectionmapalpha")) && game["radarsectionmapalpha"].IsInt())
    {
        CSetting::m_Settings.iRadarSectionMapAlpha = game["radarsectionmapalpha"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("hudalpha")) && game["hudalpha"].IsInt())
    {
        CSetting::m_Settings.iHudAlpha = game["hudalpha"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("gpscolor")) && (game["gpscolor"].IsUint() || game["gpscolor"].IsInt()))
    {
        CSetting::m_Settings.dwGPSColor = game["gpscolor"].IsUint() ? game["gpscolor"].GetUint() : game["gpscolor"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("gyroscope")) && (game["gyroscope"].IsBool() || game["gyroscope"].IsInt()))
    {
        CSetting::m_Settings.bGyroScope = game["gyroscope"].IsBool() ? game["gyroscope"].GetBool() : game["gyroscope"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("gyroaimsens")) && game["gyroaimsens"].IsInt())
    {
        CSetting::m_Settings.iGyroAimSensitivity = game["gyroaimsens"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("gyroonfootsens")) && game["gyroonfootsens"].IsInt())
    {
        CSetting::m_Settings.iGyroOnfootSensitivity = game["gyroonfootsens"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("gyroincarsens")) && game["gyroincarsens"].IsInt())
    {
        CSetting::m_Settings.iGyroIncarSensitivity = game["gyroincarsens"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("gyromode")) && game["gyromode"].IsInt())
    {
        CSetting::m_Settings.iGyroMode = game["gyromode"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setjiggle")) && (game["setjiggle"].IsBool() || game["setjiggle"].IsInt()))
    {
        CSetting::m_Settings.bJiggle = game["setjiggle"].IsBool() ? game["setjiggle"].GetBool() : game["setjiggle"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setmicstyle")) && (game["setmicstyle"].IsBool() || game["setmicstyle"].IsInt()))
    {
        CSetting::m_Settings.bMicStyle = game["setmicstyle"].IsBool() ? game["setmicstyle"].GetBool() : game["setmicstyle"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("weapontag")) && (game["weapontag"].IsBool() || game["weapontag"].IsInt()))
    {
        CSetting::m_Settings.bWeaponTag = game["weapontag"].IsBool() ? game["weapontag"].GetBool() : game["weapontag"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("fogdistance")) && game["fogdistance"].IsFloat())
    {
        CSetting::m_Settings.fFogDistance = game["fogdistance"].GetFloat();
    }

    if (game.HasMember(OBFUSCATE("shadowdistance")))
    {
        const auto& val = game["shadowdistance"];
        if (val.IsFloat()) CSetting::m_Settings.fShadowDistance = val.GetFloat();
        else if (val.IsInt()) CSetting::m_Settings.fShadowDistance = static_cast<float>(val.GetInt());
    }

    if (game.HasMember(OBFUSCATE("radarzoom")))
    {
        const auto& val = game["radarzoom"];
        if (val.IsFloat()) CSetting::m_Settings.fRadarZoom = val.GetFloat();
        else if (val.IsInt()) CSetting::m_Settings.fRadarZoom = static_cast<float>(val.GetInt());
    }

    if (game.HasMember(OBFUSCATE("setweaponswitch")) && (game["setweaponswitch"].IsBool() || game["setweaponswitch"].IsInt()))
    {
        CSetting::m_Settings.bSwapWeapon = game["setweaponswitch"].IsBool() ? game["setweaponswitch"].GetBool() : game["setweaponswitch"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setvehinfostyle")) && (game["setvehinfostyle"].IsBool() || game["setvehinfostyle"].IsInt()))
    {
        CSetting::m_Settings.bVehicleInfoStyle = game["setvehinfostyle"].IsBool() ? game["setvehinfostyle"].GetBool() : game["setvehinfostyle"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setcrouchstyle")) && (game["setcrouchstyle"].IsBool() || game["setcrouchstyle"].IsInt()))
    {
        CSetting::m_Settings.iSetCrouchStyle = game["setcrouchstyle"].IsBool() ? game["setcrouchstyle"].GetBool() : game["setcrouchstyle"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setradartouchstyle")) && (game["setradartouchstyle"].IsBool() || game["setradartouchstyle"].IsInt()))
    {
        CSetting::m_Settings.iSetRadarTouchStyle = game["setradartouchstyle"].IsBool() ? game["setradartouchstyle"].GetBool() : game["setradartouchstyle"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("setheadmove")) && (game["setheadmove"].IsBool() || game["setheadmove"].IsInt()))
    {
        CSetting::m_Settings.bHeadmove = game["setheadmove"].IsBool() ? game["setheadmove"].GetBool() : game["setheadmove"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("cameratoggle")) && (game["cameratoggle"].IsBool() || game["cameratoggle"].IsInt()))
    {
        CSetting::m_Settings.bCameraToggleStyle = game["cameratoggle"].IsBool() ? game["cameratoggle"].GetBool() : game["cameratoggle"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("cameraplanestyle")) && (game["cameraplanestyle"].IsBool() || game["cameraplanestyle"].IsInt()))
    {
        CSetting::m_Settings.bCameraPlaneStyle = game["cameraplanestyle"].IsBool() ? game["cameraplanestyle"].GetBool() : game["cameraplanestyle"].GetInt();
    }

    if (game.HasMember(OBFUSCATE("pccamera")) && (game["pccamera"].IsBool() || game["pccamera"].IsInt()))
    {
        CSetting::m_Settings.bPCCamera = game["pccamera"].IsBool() ? game["pccamera"].GetBool() : game["pccamera"].GetInt();
    }
}

void CSetting::ApplyVoiceSettings(const Value& sound)
{
    if (sound.HasMember(OBFUSCATE("setsoundvolume")) && sound["setsoundvolume"].IsInt())
    {
        CSetting::m_Settings.iSetSoundVolume = sound["setsoundvolume"].GetInt();
    }

    if (sound.HasMember(OBFUSCATE("setvolume")) && sound["setvolume"].IsInt())
    {
        CSetting::m_Settings.iSetMicroVolume = sound["setvolume"].GetInt();
    }

    if (sound.HasMember(OBFUSCATE("setvolumesmooth")) && sound["setvolumesmooth"].IsBool())
    {
        CSetting::m_Settings.bVolumeSmooth = sound["setvolumesmooth"].GetBool();
    }

    if (sound.HasMember(OBFUSCATE("sethighpassfilter")) && sound["sethighpassfilter"].IsBool())
    {
        CSetting::m_Settings.bHighPassFilter = sound["sethighpassfilter"].GetBool();
    }
}

void CSetting::ApplySettings(Document& doc)
{
    if (doc.HasMember(OBFUSCATE("client")) && doc["client"].IsObject() && doc["client"].HasMember(OBFUSCATE("settings")) && doc["client"]["settings"].IsObject())
    {
        CSetting::ApplyClientSettings(doc["client"]["settings"]);
    }

    if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
    {
        CSetting::ApplyGuiSettings(doc["gui"]);
    }

    if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
    {
        CSetting::ApplyGameSettings(doc["game"]);
    }

    if (doc.HasMember(OBFUSCATE("sound")) && doc["sound"].IsObject())
    {
        CSetting::ApplyVoiceSettings(doc["sound"]);
    }
}

void CSetting::WriteIntValue(const char* key, int value, bool writefile)
{
    if (!key) {
        return;
    }

    Document doc;
    if (!CSetting::PrepareWrite(doc, writefile)) return;

    if (!strcmp(key, OBFUSCATE("FontOutline")))
    {
        CSetting::m_Settings.iFontOutline = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["FontOutline"].SetInt(CSetting::m_Settings.iFontOutline);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("Language")))
    {
        CSetting::m_Settings.iLanguage = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["language"].SetInt(CSetting::m_Settings.iLanguage);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("Shadow")))
    {
        CSetting::m_Settings.iShadow = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["shadow"].SetInt(CSetting::m_Settings.iShadow);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("AspectRatio")))
    {
        CSetting::m_Settings.iSetAspectratio = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setaspectratio"].SetInt(CSetting::m_Settings.iSetAspectratio);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("WidgetAlpha")))
    {
        CSetting::m_Settings.iSetButtonAlpha = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setbuttonalpha"].SetInt(CSetting::m_Settings.iSetButtonAlpha);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("OnfootSensitivity")))
    {
        CSetting::m_Settings.iSetSensitivity = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setsensitivity"].SetInt(CSetting::m_Settings.iSetSensitivity);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("AimSensitivity")))
    {
        CSetting::m_Settings.iSetZoomSensitivity = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setzoomsensitivity"].SetInt(CSetting::m_Settings.iSetZoomSensitivity);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("IncarSensitivity")))
    {
        CSetting::m_Settings.iSetIncarSensitivity = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setincarsensitivity"].SetInt(CSetting::m_Settings.iSetIncarSensitivity);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("GyroAimSensitivity")))
    {
        CSetting::m_Settings.iGyroAimSensitivity = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["gyroaimsens"].SetInt(CSetting::m_Settings.iGyroAimSensitivity);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("GyroOnfootSensitivity")))
    {
        CSetting::m_Settings.iGyroOnfootSensitivity = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["gyroonfootsens"].SetInt(CSetting::m_Settings.iGyroOnfootSensitivity);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("GyroIncarSensitivity")))
    {
        CSetting::m_Settings.iGyroIncarSensitivity = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["gyroincarsens"].SetInt(CSetting::m_Settings.iGyroIncarSensitivity);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("GyroMode")))
    {
        CSetting::m_Settings.iGyroMode = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["gyromode"].SetInt(CSetting::m_Settings.iGyroMode);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("SetFps")))
    {
        CSetting::m_Settings.iSetFps = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setfps"].SetInt(CSetting::m_Settings.iSetFps);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("SetFov")))
    {
        CSetting::m_Settings.iSetFov = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setfov"].SetInt(CSetting::m_Settings.iSetFov);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("RadarAlpha")))
    {
        CSetting::m_Settings.iRadarAlpha = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["radaralpha"].SetInt(CSetting::m_Settings.iRadarAlpha);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("RadarSectionMapAlpha")))
    {
        CSetting::m_Settings.iRadarSectionMapAlpha = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["radarsectionmapalpha"].SetInt(CSetting::m_Settings.iRadarSectionMapAlpha);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("HudAlpha")))
    {
        CSetting::m_Settings.iHudAlpha = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["hudalpha"].SetInt(CSetting::m_Settings.iHudAlpha);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("GpsColor")))
    {
        CSetting::m_Settings.dwGPSColor = static_cast<uint32_t>(value);

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["gpscolor"].SetUint(CSetting::m_Settings.dwGPSColor);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("CrouchStyle")))
    {
        CSetting::m_Settings.iSetCrouchStyle = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setcrouchstyle"].SetInt(CSetting::m_Settings.iSetCrouchStyle);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("RadartouchStyle")))
    {
        CSetting::m_Settings.iSetRadarTouchStyle = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setradartouchstyle"].SetInt(CSetting::m_Settings.iSetRadarTouchStyle);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("SoundVolume")))
    {
        CSetting::m_Settings.iSetSoundVolume = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("sound")) && doc["sound"].IsObject())
            {
                doc["sound"]["setsoundvolume"].SetInt(CSetting::m_Settings.iSetSoundVolume);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("MicroPhoneVolume")))
    {
        CSetting::m_Settings.iSetMicroVolume = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("sound")) && doc["sound"].IsObject())
            {
                doc["sound"]["setvolume"].SetInt(CSetting::m_Settings.iSetMicroVolume);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("SampVersion")))
    {
        CSetting::m_Settings.sampVersion = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("client")) && doc["client"].HasMember(OBFUSCATE("settings")) && doc["client"]["settings"].IsObject())
            {
                doc["client"]["settings"]["sampversion"].SetInt(CSetting::m_Settings.sampVersion);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ServerPort")))
    {
        CSetting::m_Settings.port = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("client")) && doc["client"].HasMember(OBFUSCATE("settings")) && doc["client"]["settings"].IsObject())
            {
                doc["client"]["settings"]["ServerPort"].SetInt(CSetting::m_Settings.port);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("LastServer")))
    {
        CSetting::m_Settings.last_server = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("client")) && doc["client"].HasMember(OBFUSCATE("settings")) && doc["client"]["settings"].IsObject())
            {
                doc["client"]["settings"]["LastServer"].SetInt(CSetting::m_Settings.last_server);
            }
        }
    }
    else
    {
        return;
    }

    if (writefile)
    {
        CSetting::WriteJsonToFile(doc);
    }
}

void CSetting::WriteBoolValue(const char* key, bool value, bool writefile)
{
    if (!key) {
        return;
    }

    Document doc;
    if (!CSetting::PrepareWrite(doc, writefile)) return;

    if (!strcmp(key, OBFUSCATE("TimeStamp")))
    {
        CSetting::m_Settings.bTimeStamp = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["timestamp"].SetBool(CSetting::m_Settings.bTimeStamp);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("Jiggle")))
    {
        CSetting::m_Settings.bJiggle = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setjiggle"].SetBool(CSetting::m_Settings.bJiggle);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("GyroScope")))
    {
        CSetting::m_Settings.bGyroScope = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["gyroscope"].SetBool(CSetting::m_Settings.bGyroScope);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("MicStyle")))
    {
        CSetting::m_Settings.bMicStyle = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setmicstyle"].SetBool(CSetting::m_Settings.bMicStyle);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("WeaponTag")))
    {
        CSetting::m_Settings.bWeaponTag = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["weapontag"].SetBool(CSetting::m_Settings.bWeaponTag);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("SwapWeapon")))
    {
        CSetting::m_Settings.bSwapWeapon = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setweaponswitch"].SetBool(CSetting::m_Settings.bSwapWeapon);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("VehicleInfoStyle")))
    {
        CSetting::m_Settings.bVehicleInfoStyle = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setvehinfostyle"].SetBool(CSetting::m_Settings.bVehicleInfoStyle);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("Headmove")))
    {
        CSetting::m_Settings.bHeadmove = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["setheadmove"].SetBool(CSetting::m_Settings.bHeadmove);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("CameraToggleStyle")))
    {
        CSetting::m_Settings.bCameraToggleStyle = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["cameratoggle"].SetBool(CSetting::m_Settings.bCameraToggleStyle);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("CameraPlaneStyle")))
    {
        CSetting::m_Settings.bCameraPlaneStyle = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["cameraplanestyle"].SetBool(CSetting::m_Settings.bCameraPlaneStyle);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("PCCamera")))
    {
        CSetting::m_Settings.bPCCamera = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["pccamera"].SetBool(CSetting::m_Settings.bPCCamera);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ShowFps")))
    {
        CSetting::m_Settings.bShowFps = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["showfps"].SetBool(CSetting::m_Settings.bShowFps);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ShowMem")))
    {
        CSetting::m_Settings.bShowMem = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["showmem"].SetBool(CSetting::m_Settings.bShowMem);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ShowKeyF4")))
    {
        CSetting::m_Settings.bShowKeyF4 = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["keyf4"].SetBool(CSetting::m_Settings.bShowKeyF4);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ShowKeyUPDOWN")))
    {
        CSetting::m_Settings.bShowKeyUPDOWN = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["keyupdown"].SetBool(CSetting::m_Settings.bShowKeyUPDOWN);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ShowKeyLEFTRIGHT")))
    {
        CSetting::m_Settings.bShowKeyLEFTRIGHT = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["keyleftright"].SetBool(CSetting::m_Settings.bShowKeyLEFTRIGHT);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("VolumeSmooth")))
    {
        CSetting::m_Settings.bVolumeSmooth = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("sound")) && doc["sound"].IsObject())
            {
                doc["sound"]["setvolumesmooth"].SetBool(CSetting::m_Settings.bVolumeSmooth);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("HighPassFilter")))
    {
        CSetting::m_Settings.bHighPassFilter = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("sound")) && doc["sound"].IsObject())
            {
                doc["sound"]["sethighpassfilter"].SetBool(CSetting::m_Settings.bHighPassFilter);
            }
        }
    }
    else
    {
        return;
    }

    if (writefile)
    {
        CSetting::WriteJsonToFile(doc);
    }
}

void CSetting::WriteFloatValue(const char* key, float value, bool writefile)
{
    if (!key || !std::isfinite(value)) {
        return;
    }

    Document doc;
    if (!CSetting::PrepareWrite(doc, writefile)) return;

    if (!strcmp(key, OBFUSCATE("ChatPosX")))
    {
        CSetting::m_Settings.fChatPosX = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["ChatPosX"].SetFloat(CSetting::m_Settings.fChatPosX);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ChatPosY")))
    {
        CSetting::m_Settings.fChatPosY = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["ChatPosY"].SetFloat(CSetting::m_Settings.fChatPosY);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ChatSizeX")))
    {
        CSetting::m_Settings.fChatSizeX = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["ChatSizeX"].SetFloat(CSetting::m_Settings.fChatSizeX);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ChatSizeY")))
    {
        CSetting::m_Settings.fChatSizeY = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["ChatSizeY"].SetFloat(CSetting::m_Settings.fChatSizeY);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("FontSize")))
    {
        CSetting::m_Settings.fFontSize = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["FontSize"].SetFloat(CSetting::m_Settings.fFontSize);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("HealthBarWidth")))
    {
        CSetting::m_Settings.fHealthBarWidth = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["HealthBarWidth"].SetFloat(CSetting::m_Settings.fHealthBarWidth);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("HealthBarHeight")))
    {
        CSetting::m_Settings.fHealthBarHeight = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["HealthBarHeight"].SetFloat(CSetting::m_Settings.fHealthBarHeight);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonEnterPassengerX")))
    {
        CSetting::m_Settings.fButtonEnterPassengerX = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["EnterPassengerX"].SetFloat(CSetting::m_Settings.fButtonEnterPassengerX);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonEnterPassengerY")))
    {
        CSetting::m_Settings.fButtonEnterPassengerY = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["EnterPassengerY"].SetFloat(CSetting::m_Settings.fButtonEnterPassengerY);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonEnterPassengerSize")))
    {
        CSetting::m_Settings.fButtonEnterPassengerSize = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["EnterPassengerSize"].SetFloat(CSetting::m_Settings.fButtonEnterPassengerSize);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonMicrophoneX")))
    {
        CSetting::m_Settings.fButtonMicrophoneX = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["MicrophoneX"].SetFloat(CSetting::m_Settings.fButtonMicrophoneX);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonMicrophoneY")))
    {
        CSetting::m_Settings.fButtonMicrophoneY = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["MicrophoneY"].SetFloat(CSetting::m_Settings.fButtonMicrophoneY);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonMicrophoneSize")))
    {
        CSetting::m_Settings.fButtonMicrophoneSize = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["MicrophoneSize"].SetFloat(CSetting::m_Settings.fButtonMicrophoneSize);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonCameraCycleX")))
    {
        CSetting::m_Settings.fButtonCameraCycleX = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["CameraCycleX"].SetFloat(CSetting::m_Settings.fButtonCameraCycleX);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonCameraCycleY")))
    {
        CSetting::m_Settings.fButtonCameraCycleY = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["CameraCycleY"].SetFloat(CSetting::m_Settings.fButtonCameraCycleY);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ButtonCameraCycleSize")))
    {
        CSetting::m_Settings.fButtonCameraCycleSize = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["CameraCycleSize"].SetFloat(CSetting::m_Settings.fButtonCameraCycleSize);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("JavaChatPosX")))
    {
        CSetting::m_Settings.fJavaChatPosX = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["JavaChatPosX"].SetFloat(CSetting::m_Settings.fJavaChatPosX);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("FogDistance")))
    {
        CSetting::m_Settings.fFogDistance = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["fogdistance"].SetFloat(CSetting::m_Settings.fFogDistance);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("ShadowDistance")))
    {
        CSetting::m_Settings.fShadowDistance = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["shadowdistance"].SetFloat(CSetting::m_Settings.fShadowDistance);
            }
        }
    }
    else if (!strcmp(key, OBFUSCATE("RadarZoom")))
    {
        CSetting::m_Settings.fRadarZoom = value;

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("game")) && doc["game"].IsObject())
            {
                doc["game"]["radarzoom"].SetFloat(CSetting::m_Settings.fRadarZoom);
            }
        }
    }
    else
    {
        return;
    }

    if (writefile)
    {
        CSetting::WriteJsonToFile(doc);
    }
}

void CSetting::WriteStringValue(const char* key, const char* value, bool writefile)
{
    if (!key || !value) {
        return;
    }

    Document doc;
    if (!CSetting::PrepareWrite(doc, writefile)) return;

    if (!strcmp(key, OBFUSCATE("Crosshair")))
    {
        CopySettingString(CSetting::m_Settings.szCrosshair, value);

        if (writefile)
        {
            if (doc.HasMember(OBFUSCATE("gui")) && doc["gui"].IsObject())
            {
                doc["gui"]["Crosshair"].SetString(m_Settings.szCrosshair, doc.GetAllocator());
            }
        }
    }
    else
    {

        return;
    }

    if (writefile)
    {
        CSetting::WriteJsonToFile(doc);
    }
}

const char* CSetting::GetStringValue(const char* key)
{
    if (!key) {
        return OBFUSCATE("");
    }

    if (!strcmp(key, OBFUSCATE("NickName")))
    {
        return CSetting::m_Settings.szNickName;
    }

    if (!strcmp(key, OBFUSCATE("ServerIp")))
    {
        return CSetting::m_Settings.szIp;
    }

    if (!strcmp(key, OBFUSCATE("ServerPassword")))
    {
        return CSetting::m_Settings.szPassword;
    }

    if (!strcmp(key, OBFUSCATE("Font")))
    {
        return CSetting::m_Settings.szFont;
    }

    if (!strcmp(key, OBFUSCATE("Crosshair")))
    {
        return CSetting::m_Settings.szCrosshair;
    }

    return OBFUSCATE("");
}

int CSetting::GetIntValue(const char* key)
{
    if (!key) {
        return 0;
    }

    if (!strcmp(key, OBFUSCATE("ServerPort")))
    {
        return CSetting::m_Settings.port;
    }

    if (!strcmp(key, OBFUSCATE("LastServer")))
    {
        return CSetting::m_Settings.last_server;
    }

    if (!strcmp(key, OBFUSCATE("SampVersion")))
    {
        return CSetting::m_Settings.sampVersion;
    }

    if (!strcmp(key, OBFUSCATE("FontOutline")))
    {
        return CSetting::m_Settings.iFontOutline;
    }

    if (!strcmp(key, OBFUSCATE("Language")))
    {
        return CSetting::m_Settings.iLanguage;
    }

    if (!strcmp(key, OBFUSCATE("Shadow")))
    {
        return CSetting::m_Settings.iShadow;
    }

    if (!strcmp(key, OBFUSCATE("AspectRatio")))
    {
        return CSetting::m_Settings.iSetAspectratio;
    }

    if (!strcmp(key, OBFUSCATE("WidgetAlpha")))
    {
        return CSetting::m_Settings.iSetButtonAlpha;
    }

    if (!strcmp(key, OBFUSCATE("OnfootSensitivity")))
    {
        return CSetting::m_Settings.iSetSensitivity;
    }

    if (!strcmp(key, OBFUSCATE("AimSensitivity")))
    {
        return CSetting::m_Settings.iSetZoomSensitivity;
    }

    if (!strcmp(key, OBFUSCATE("IncarSensitivity")))
    {
        return CSetting::m_Settings.iSetIncarSensitivity;
    }

    if (!strcmp(key, OBFUSCATE("GyroAimSensitivity")))
    {
        return CSetting::m_Settings.iGyroAimSensitivity;
    }

    if (!strcmp(key, OBFUSCATE("GyroOnfootSensitivity")))
    {
        return CSetting::m_Settings.iGyroOnfootSensitivity;
    }

    if (!strcmp(key, OBFUSCATE("GyroIncarSensitivity")))
    {
        return CSetting::m_Settings.iGyroIncarSensitivity;
    }

    if (!strcmp(key, OBFUSCATE("GyroMode")))
    {
        return CSetting::m_Settings.iGyroMode;
    }

    if (!strcmp(key, OBFUSCATE("SetFps")))
    {
        return CSetting::m_Settings.iSetFps;
    }

    if (!strcmp(key, OBFUSCATE("SetFov")))
    {
        return CSetting::m_Settings.iSetFov;
    }

    if (!strcmp(key, OBFUSCATE("RadarAlpha")))
    {
        return CSetting::m_Settings.iRadarAlpha;
    }

    if (!strcmp(key, OBFUSCATE("RadarSectionMapAlpha")))
    {
        return CSetting::m_Settings.iRadarSectionMapAlpha;
    }

    if (!strcmp(key, OBFUSCATE("HudAlpha")))
    {
        return CSetting::m_Settings.iHudAlpha;
    }

    if (!strcmp(key, OBFUSCATE("GpsColor")))
    {
        return static_cast<int>(CSetting::m_Settings.dwGPSColor);
    }

    if (!strcmp(key, OBFUSCATE("CrouchStyle")))
    {
        return CSetting::m_Settings.iSetCrouchStyle;
    }

    if (!strcmp(key, OBFUSCATE("RadartouchStyle")))
    {
        return CSetting::m_Settings.iSetRadarTouchStyle;
    }

    if (!strcmp(key, OBFUSCATE("SoundVolume")))
    {
        return CSetting::m_Settings.iSetSoundVolume;
    }

    if (!strcmp(key, OBFUSCATE("MicroPhoneVolume")))
    {
        return CSetting::m_Settings.iSetMicroVolume;
    }

    return 0;
}

bool CSetting::GetBoolValue(const char* key)
{
    if (!key) {
        return false;
    }

    if (!strcmp(key, OBFUSCATE("TimeStamp")))
    {
        return CSetting::m_Settings.bTimeStamp;
    }
    else if (!strcmp(key, OBFUSCATE("Jiggle")))
    {
        return CSetting::m_Settings.bJiggle;
    }
    else if (!strcmp(key, OBFUSCATE("MicStyle")))
    {
        return CSetting::m_Settings.bMicStyle;
    }
    else if (!strcmp(key, OBFUSCATE("WeaponTag")))
    {
        return CSetting::m_Settings.bWeaponTag;
    }
    else if (!strcmp(key, OBFUSCATE("SwapWeapon")))
    {
        return CSetting::m_Settings.bSwapWeapon;
    }
    else if (!strcmp(key, OBFUSCATE("VehicleInfoStyle")))
    {
        return CSetting::m_Settings.bVehicleInfoStyle;
    }
    else if (!strcmp(key, OBFUSCATE("Headmove")))
    {
        return CSetting::m_Settings.bHeadmove;
    }
    else if (!strcmp(key, OBFUSCATE("CameraToggleStyle")))
    {
        return CSetting::m_Settings.bCameraToggleStyle;
    }
    else if (!strcmp(key, OBFUSCATE("CameraPlaneStyle")))
    {
        return CSetting::m_Settings.bCameraPlaneStyle;
    }
    else if (!strcmp(key, OBFUSCATE("PCCamera")))
    {
        return CSetting::m_Settings.bPCCamera;
    }
    else if (!strcmp(key, OBFUSCATE("GyroScope")))
    {
        return CSetting::m_Settings.bGyroScope;
    }
    else if (!strcmp(key, OBFUSCATE("ShowFps")))
    {
        return CSetting::m_Settings.bShowFps;
    }
    else if (!strcmp(key, OBFUSCATE("ShowMem")))
    {
        return CSetting::m_Settings.bShowMem;
    }
    else if (!strcmp(key, OBFUSCATE("ShowKeyF4")))
    {
        return CSetting::m_Settings.bShowKeyF4;
    }
    else if (!strcmp(key, OBFUSCATE("ShowKeyUPDOWN")))
    {
        return CSetting::m_Settings.bShowKeyUPDOWN;
    }
    else if (!strcmp(key, OBFUSCATE("ShowKeyLEFTRIGHT")))
    {
        return CSetting::m_Settings.bShowKeyLEFTRIGHT;
    }
    else if (!strcmp(key, OBFUSCATE("VolumeSmooth")))
    {
        return CSetting::m_Settings.bVolumeSmooth;
    }
    else if (!strcmp(key, OBFUSCATE("HighPassFilter")))
    {
        return CSetting::m_Settings.bHighPassFilter;
    }
    else
    {
        return false;
    }
}

float CSetting::GetFloatValue(const char* key)
{
    if (!key) {
        return 0.0f;
    }

    if (!strcmp(key, OBFUSCATE("ChatPosX")))
    {
        return CSetting::m_Settings.fChatPosX;
    }
    else if (!strcmp(key, OBFUSCATE("ChatPosY")))
    {
        return CSetting::m_Settings.fChatPosY;
    }
    else if (!strcmp(key, OBFUSCATE("ChatSizeX")))
    {
        return CSetting::m_Settings.fChatSizeX;
    }
    else if (!strcmp(key, OBFUSCATE("ChatSizeY")))
    {
        return CSetting::m_Settings.fChatSizeY;
    }
    else if (!strcmp(key, OBFUSCATE("FontSize")))
    {
        return CSetting::m_Settings.fFontSize;
    }
    else if (!strcmp(key, OBFUSCATE("HealthBarWidth")))
    {
        return CSetting::m_Settings.fHealthBarWidth;
    }
    else if (!strcmp(key, OBFUSCATE("HealthBarHeight")))
    {
        return CSetting::m_Settings.fHealthBarHeight;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonEnterPassengerX")))
    {
        return CSetting::m_Settings.fButtonEnterPassengerX;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonEnterPassengerY")))
    {
        return CSetting::m_Settings.fButtonEnterPassengerY;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonEnterPassengerSize")))
    {
        return CSetting::m_Settings.fButtonEnterPassengerSize;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonMicrophoneX")))
    {
        return CSetting::m_Settings.fButtonMicrophoneX;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonMicrophoneY")))
    {
        return CSetting::m_Settings.fButtonMicrophoneY;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonMicrophoneSize")))
    {
        return CSetting::m_Settings.fButtonMicrophoneSize;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonCameraCycleX")))
    {
        return CSetting::m_Settings.fButtonCameraCycleX;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonCameraCycleY")))
    {
        return CSetting::m_Settings.fButtonCameraCycleY;
    }
    else if (!strcmp(key, OBFUSCATE("ButtonCameraCycleSize")))
    {
        return CSetting::m_Settings.fButtonCameraCycleSize;
    }
    else if (!strcmp(key, OBFUSCATE("JavaChatPosX")))
    {
        return CSetting::m_Settings.fJavaChatPosX;
    }
    else if (!strcmp(key, OBFUSCATE("FogDistance")))
    {
        return CSetting::m_Settings.fFogDistance;
    }
    else if (!strcmp(key, OBFUSCATE("ShadowDistance")))
    {
        return CSetting::m_Settings.fShadowDistance;
    }
    else if (!strcmp(key, OBFUSCATE("RadarZoom")))
    {
        return CSetting::m_Settings.fRadarZoom;
    }
    else
    {
        return 0.0f;
    }
}

bool CSetting::PrepareWrite(Document& doc, bool writefile)
{
    if (!writefile) return true;
    const auto error = CSetting::ReadJsonFromFile(doc);
    if (error && error != std::errc::no_such_file_or_directory) return false;

    Document defaults;
    CSetting::CreateDefault(defaults);
    if (error) doc.Swap(defaults);
    else MergeSettingsDefaults(doc, defaults, doc.GetAllocator());
    return true;
}

bool CSetting::WriteJsonToFile(const Document& doc)
{
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    if (doc.Accept(writer) && morgan::fileio::CStorage::WriteSampSettings({buffer.GetString(), buffer.GetSize()})) return true;

    CNecessary::Log(OBFUSCATE("Cannot write SAMP/settings.json"));
    return false;
}

std::error_code CSetting::ReadJsonFromFile(Document& doc)
{
    std::string bytes;
    if (const auto error = morgan::fileio::CStorage::ReadSampSettings(bytes)) return error;
    if (!morgan::fileio::storage::ParseSampSettings(bytes, doc)) return std::make_error_code(std::errc::invalid_argument);

    return {};
}