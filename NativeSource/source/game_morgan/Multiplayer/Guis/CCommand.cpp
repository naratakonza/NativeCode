#include "../../../../Main.h"
#include "CCommand.h"
#include "../CMultiPlayer.h"
#include "../CNetGame.h"

#include <strings.h>

std::vector<ChatCommand> CCommand::commandList;

void CCommand::Initialise()
{
    CNecessary::Log(OBFUSCATE("CCommand Initialise"));

    CCommand::commandList.clear();
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
