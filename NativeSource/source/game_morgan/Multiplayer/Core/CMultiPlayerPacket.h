#pragma once

class CMultiPlayerPacket
{
public:
    static bool SendChatMessage(const char* text);
    static bool SendChatCommand(const char* text);

private:
    static bool SendChatRPC(const char* text, bool command);
};
