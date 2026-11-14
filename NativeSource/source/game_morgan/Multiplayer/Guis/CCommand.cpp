#include "../../../../Main.h"
#include "CCommand.h"
#include "CGui.h"
#include "../../CAspectRatio.h"
#include "../../CDraw.h"
#include "../../CSensitivity.h"
#include "../../Enums/eMaxConfig.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"

std::vector<ChatCommand> CCommand::commandList;

void CCommand::Initialise()
{
    CNecessary::Log(OBFUSCATE("CCommand Initialise"));

    CCommand::commandList.clear();

    CCommand::Add(OBFUSCATE("/apr"), CCommand::SetAspectRatio);
    CCommand::Add(OBFUSCATE("/fpslimit"), CCommand::SetFramerateLimit);
    CCommand::Add(OBFUSCATE("/fov"), CCommand::SetFov);
    CCommand::Add(OBFUSCATE("/sens"), CCommand::SetSensitivity);
    CCommand::Add(OBFUSCATE("/aimsens"), CCommand::SetAimWeaponSensitivity);
    CCommand::Add(OBFUSCATE("/incarsens"), CCommand::SetIncarSensitivity);
    CCommand::Add(OBFUSCATE("/rec"), CCommand::Reconnect);
    CCommand::Add(OBFUSCATE("/reconnect"), CCommand::Reconnect);
    CCommand::Add(OBFUSCATE("/q"), CCommand::QuitGame);
    CCommand::Add(OBFUSCATE("/quit"), CCommand::QuitGame);
}

void CCommand::Add(const char* cmd, CommandCallback callback)
{
    CCommand::commandList.push_back({cmd, callback});
}

bool CCommand::Process(const std::string& input)
{
    if (input.empty() || input[0] != '/') {
        return false;
    }

    const char* data = input.c_str() + 1;
    const char* cmsStart = data;

    while (*data && *data != ' ')
    {
        ++data;
    }

    size_t cmdLen = data - cmsStart;
    if (cmdLen == 0) {
        return false;
    }

    for (const auto& entry : CCommand::commandList)
    {
        if (!entry.name.empty())
        {
            const char* regCmd = entry.name.c_str() + 1;
            if (strlen(regCmd) == cmdLen && strncasecmp(regCmd, cmsStart, cmdLen) == 0)
            {
                std::vector<std::string> args;
                args.emplace_back(cmsStart, cmdLen);

                if (*data == ' ')
                {
                    const char* arg = data + 1;
                    while (*arg)
                    {
                        while (*arg == ' ')
                        {
                            ++arg;
                        }

                        if (*arg == '\0') {
                            break;
                        }

                        const char* start = arg;
                        while (*arg != ' ' && *arg != '\0')
                        {
                            ++arg;
                        }

                        args.emplace_back(start, arg - start);
                    }
                }

                entry.callback(args);
                return true;
            }
        }
    }

    return false;
}

void CCommand::SetAspectRatio(const std::vector<std::string>& args)
{
    if (!CSetting::GetConfig() || !CGui::GetChat()) {
        return;
    }

    if (args.size() >= 2)
    {
        const char* value = args[1].c_str();
        char* end = nullptr;
        const long type = std::strtol(value, &end, 10);

        if (end != value && *end == '\0' && type >= eAspectRatioType::ASPECTRATIO_DEFAULT && type <= eAspectRatioType::ASPECTRATIO_32_9)
        {
            CAspectRatio::SetSize((eAspectRatioType)type);
            CSetting::GetConfig()->WriteIntValue(OBFUSCATE("AspectRatio"), static_cast<int>(type));

            CGui::GetChat()->AddInfoMessage(OBFUSCATE("-> AspectRatio: %u"), static_cast<unsigned>(type));
        }
        else
        {
            CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("-> AspectRatio: valid amounts are 0-7"));
        }
    }
    else
    {
        CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("Usage: /apr <value>"));
    }
}

void CCommand::SetFramerateLimit(const std::vector<std::string>& args)
{
    if (!CSetting::GetConfig() || !CGui::GetChat()) {
        return;
    }

    if (args.size() >= 2)
    {
        const char* value = args[1].c_str();
        char* end = nullptr;
        const long fps = std::strtol(value, &end, 10);

        if (end != value && *end == '\0' && fps >= 0 && fps <= 2)
        {
            CGame::SetFramerate(static_cast<std::uint8_t>(fps));
            CSetting::GetConfig()->WriteIntValue(OBFUSCATE("SetFps"), static_cast<int>(fps));

            CGui::GetChat()->AddInfoMessage(OBFUSCATE("-> Frame Limiter: %u"), static_cast<unsigned>(fps));
        }
        else
        {
            CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("-> Frame Limiter: valid amounts are 0-2"));
        }
    }
    else
    {
        CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("Usage: /fpslimit <value>"));
    }
}

void CCommand::SetFov(const std::vector<std::string>& args)
{
    if (!CSetting::GetConfig() || !CGui::GetChat()) {
        return;
    }

    if (args.size() >= 2)
    {
        const char* value = args[1].c_str();
        char* end = nullptr;
        const long fov = std::strtol(value, &end, 10);

        if (end != value && *end == '\0' && fov >= 77 && fov <= eMaxConfig::MAX_FOV)
        {
            CDraw::SetFov(static_cast<float>(fov));
            CSetting::GetConfig()->WriteIntValue(OBFUSCATE("SetFov"), static_cast<int>(fov));

            CGui::GetChat()->AddInfoMessage(OBFUSCATE("-> Fov Limiter: %2.f"), CDraw::GetFov());
        }
        else
        {
            CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("-> Fov Limiter: valid amounts are 77-140"));
        }
    }
    else
    {
        CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("Usage: /fov <value>"));
    }
}

void CCommand::SetSensitivity(const std::vector<std::string>& args)
{
    if (!CSetting::GetConfig() || !CGui::GetChat()) {
        return;
    }

    if (args.size() >= 2)
    {
        const char* value = args[1].c_str();
        char* end = nullptr;
        const long type = std::strtol(value, &end, 10);

        if (end != value && *end == '\0' && type >= 0 && type <= eMaxConfig::MAX_SENSITIVITY)
        {
            CSensitivity::SetOnfootTouch(static_cast<float>(type));
            CSetting::GetConfig()->WriteIntValue(OBFUSCATE("OnfootSensitivity"), static_cast<int>(type));

            CGui::GetChat()->AddInfoMessage(OBFUSCATE("-> Sensitivity Limiter: %u"), static_cast<unsigned>(type));
        }
        else
        {
            CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("-> Sensitivity Limiter: valid amounts are 0-200"));
        }
    }
    else
    {
        CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("Usage: /sens <value>"));
    }
}

void CCommand::SetAimWeaponSensitivity(const std::vector<std::string>& args)
{
    if (!CSetting::GetConfig() || !CGui::GetChat()) {
        return;
    }

    if (args.size() >= 2)
    {
        const char* value = args[1].c_str();
        char* end = nullptr;
        const long type = std::strtol(value, &end, 10);

        if (end != value && *end == '\0' && type >= 0 && type <= eMaxConfig::MAX_SENSITIVITY)
        {
            CSensitivity::SetAimTouch(static_cast<float>(type));
            CSetting::GetConfig()->WriteIntValue(OBFUSCATE("AimSensitivity"), static_cast<int>(type));

            CGui::GetChat()->AddInfoMessage(OBFUSCATE("-> AimWeaponSensitivity Limiter: %u"), static_cast<unsigned>(type));
        }
        else
        {
            CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("-> AimWeaponSensitivity Limiter: valid amounts are 0-200"));
        }
    }
    else
    {
        CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("Usage: /aimsens <value>"));
    }
}

void CCommand::SetIncarSensitivity(const std::vector<std::string>& args)
{
    if (!CSetting::GetConfig() || !CGui::GetChat()) {
        return;
    }

    if (args.size() >= 2)
    {
        const char* value = args[1].c_str();
        char* end = nullptr;
        const long type = std::strtol(value, &end, 10);

        if (end != value && *end == '\0' && type >= 0 && type <= eMaxConfig::MAX_SENSITIVITY)
        {
            CSensitivity::SetIncarTouch(static_cast<float>(type));
            CSetting::GetConfig()->WriteIntValue(OBFUSCATE("IncarSensitivity"), static_cast<int>(type));

            CGui::GetChat()->AddInfoMessage(OBFUSCATE("-> IncarSensitivity Limiter: %u"), static_cast<unsigned>(type));
        }
        else
        {
            CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("-> IncarSensitivity Limiter: valid amounts are 0-200"));
        }
    }
    else
    {
        CGui::GetChat()->AddDebugMessage(2, OBFUSCATE("Usage: /incarsens <value>"));
    }
}

void CCommand::Reconnect(const std::vector<std::string>&)
{
    if (CMultiPlayer::GetActiveNetwork())
    {
        CMultiPlayer::GetActiveNetwork()->SetNetworkState(eNetworkState::GAMESTATE_RESTARTING);
        CMultiPlayer::Reconnect();
    }
}

void CCommand::QuitGame(const std::vector<std::string>&)
{
    ExitGame();
}