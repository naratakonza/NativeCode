#pragma once

#include "../../Enums/eChatMessageType.h"

#include <cstdint>
#include <string>

class CChat
{
public:
    CChat();
    ~CChat();

    void Render();
    void ProcessPendingChatInputs();
    static bool QueueInput(std::string input);
    void ChatWindowInputHandler(const char* str);
    void AddChatMessage(const char* szNick, uint32_t dwNickColor, const char* szMessage);
    void AddInfoMessage(const char* szFormat, ...);
    void AddDebugMessage(int alerttype, const char* szFormat, ...);
    void AddClientMessage(uint32_t dwColor, const char* szStr);

private:
    void AddToChatWindowBuffer(eChatMessageType type, std::string message, const char* nick, uint32_t textColor, uint32_t nickColor, int alerttype);

    uint32_t m_dwTextColor = 0xFFFFFFFF;
    uint32_t m_dwInfoColor = 0x62AA88FF;
    uint32_t m_dwDebugColor = 0xBEBEBEFF;
};
