#pragma once

class CMultiPlayerPacket
{
public:
    static void SendDialogResponse(uint16_t wDialogID, uint8_t byteButtonID, uint16_t wListBoxItem, char* szInput);
    static void SendChatCommand(const char* szCommand);
    static void SendChatMessage(const char* szMsg);
    static void UpdatePlayerScoresAndPings();
};