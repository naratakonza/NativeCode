#pragma once

class CMultiPlayerPacket
{
public:
    static void SendChatCommand(const char* szCommand);
    static void SendChatMessage(const char* szMsg);
};