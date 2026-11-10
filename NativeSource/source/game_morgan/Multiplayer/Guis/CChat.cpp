#include "../../../../Main.h"
#include "CChat.h"
#include "CCommand.h"
#include "../CMultiPlayer.h"
#include "../Core/CMultiPlayerPacket.h"
#include "../../Enums/eMaxItem.h"
#include "../../../../Launcher/MGFlutterLoadingScreen.h"

#include <queue>

static std::queue<std::string> m_ChatInputQueue;

CChat::CChat()
{
    CCommand::Initialise();
}

CChat::~CChat()
{
    m_ChatInputQueue = {};
    MGCloseFlutterChat();
}

bool CChat::QueueInput(std::string input)
{
    // Flutter and the existing CGame_Process hook both run on the iOS main thread.
    if (!pthread_main_np() || input.empty() || input.size() > eMaxItem::MAX_MESSAGE_LENGTH || input.find('\0') != std::string::npos || m_ChatInputQueue.size() >= 16) {
        return false;
    }

    m_ChatInputQueue.push(std::move(input));
    return true;
}

void CChat::ProcessPendingChatInputs()
{
    std::queue<std::string> localQueue;
    localQueue.swap(m_ChatInputQueue);
    while (!localQueue.empty())
    {
        CChat::ChatWindowInputHandler(localQueue.front().c_str());
        localQueue.pop();
    }
}

void CChat::ChatWindowInputHandler(const char* str)
{
    if (!str || !*str) {
        return;
    }

    if (*str == '/' && CCommand::Process(str)) {
        return;
    }

    const bool sent = *str == '/' ? CMultiPlayerPacket::SendChatCommand(str) : CMultiPlayerPacket::SendChatMessage(str);
    if (!sent) {
        CChat::AddInfoMessage(OBFUSCATE("Chat is unavailable until the server connection is ready."));
    }
}

void CChat::Render()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    // if (CGui::GetScoreBoard()->GetActiveScore() || CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() ||
    //     CGui::GetVoiceMenu()->GetActiveVoiceMenu() || CVoiceMenu::m_bCustomizeMode)
    // {
    //     MGSetFlutterChatVisible(false);
    //     return;
    // }

    // || CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsSpectating()
    MGSetFlutterChatVisible(true);
}

void CChat::AddChatMessage(const char* szNick, uint32_t dwNickColor, const char* szMessage)
{
    if (szMessage) {
        CChat::AddToChatWindowBuffer(CHAT_TYPE_CHAT, szMessage, szNick, CChat::m_dwTextColor, dwNickColor, 0);
    }
}

void CChat::AddInfoMessage(const char* szFormat, ...)
{
    char message[1024]{};
    va_list args;
    va_start(args, szFormat);
    std::vsnprintf(message, sizeof(message), szFormat, args);
    va_end(args);
    CChat::AddToChatWindowBuffer(CHAT_TYPE_INFO, message, nullptr, CChat::m_dwInfoColor, 0, 0);
}

void CChat::AddDebugMessage(int alerttype, const char* szFormat, ...)
{
    char message[1024]{};
    va_list args;
    va_start(args, szFormat);
    std::vsnprintf(message, sizeof(message), szFormat, args);
    va_end(args);
    CChat::AddToChatWindowBuffer(CHAT_TYPE_DEBUG, message, nullptr, CChat::m_dwDebugColor, 0, alerttype);
}

void CChat::AddClientMessage(uint32_t dwColor, const char* szStr)
{
    if (szStr) {
        CChat::AddToChatWindowBuffer(CHAT_TYPE_INFO, szStr, nullptr, dwColor, 0, 0);
    }
}

void CChat::AddToChatWindowBuffer(eChatMessageType type, std::string message, const char* nick, uint32_t textColor, uint32_t nickColor, int alerttype)
{
    if (message.empty()) {
        return;
    }

    for (char& ch : message)
    {
        if (static_cast<unsigned char>(ch) < ' ') {
            ch = ' ';
        }
    }
    MGAddFlutterChatMessage(type == CHAT_TYPE_DEBUG ? alerttype : 0, textColor, nickColor, nick, message.c_str());
}
