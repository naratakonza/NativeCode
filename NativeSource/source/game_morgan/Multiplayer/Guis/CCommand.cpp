#include "../../../../Main.h"
#include "CCommand.h"
#include "../CMultiPlayer.h"

#include <strings.h>

std::vector<ChatCommand> CCommand::commandList;

void CCommand::Initialise()
{
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
    if (input.size() < 2 || input.front() != '/') {
        return false;
    }

    const auto end = input.find(' ');
    const auto length = end == std::string::npos ? input.size() : end;
    for (const auto& entry : CCommand::commandList)
    {
        if (entry.name.size() != length || strncasecmp(entry.name.c_str(), input.c_str(), length) != 0) {
            continue;
        }

        std::vector<std::string> args{input.substr(1, length - 1)};
        if (end != std::string::npos)
        {
            for (auto start = input.find_first_not_of(' ', end); start != std::string::npos;)
            {
                const auto next = input.find(' ', start);
                args.push_back(input.substr(start, next == std::string::npos ? next : next - start));
                start = next == std::string::npos ? next : input.find_first_not_of(' ', next);
            }
        }
        entry.callback(args);
        return true;
    }
    return false;
}

void CCommand::Reconnect(const std::vector<std::string>&)
{
    CMultiPlayer::Reconnect();
}

void CCommand::QuitGame(const std::vector<std::string>&)
{
    ExitGame();
}
