#pragma once

using CommandCallback = void (*)(const std::vector<std::string>&);

struct ChatCommand
{
    std::string name;
    CommandCallback callback;
};

class CCommand
{
public:
    static void Initialise();
    static void Add(const char* cmd, CommandCallback callback);
    static bool Process(const std::string& input);

private:
    static void Reconnect(const std::vector<std::string>& args);
    static void QuitGame(const std::vector<std::string>& args);
    static void SetAspectRatio(const std::vector<std::string>& args);
    static void SetFramerateLimit(const std::vector<std::string>& args);
    static void SetFov(const std::vector<std::string>& args);
    static void SetSensitivity(const std::vector<std::string>& args);
    static void SetAimWeaponSensitivity(const std::vector<std::string>& args);
    static void SetIncarSensitivity(const std::vector<std::string>& args);

    static std::vector<ChatCommand> commandList;
};