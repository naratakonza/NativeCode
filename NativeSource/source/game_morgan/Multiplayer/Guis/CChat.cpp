#include "../../../../Main.h"
#include "CChat.h"
#include "CGui.h"
#include "CCommand.h"
#include "../CMultiPlayer.h"
#include "../Core/CMultiPlayerPacket.h"
#include "../../Base/CPad.h"
#include "../../../../Launcher/MGFlutterLoadingScreen.h"
#include <queue>

static std::queue<std::string> m_ChatInputQueue;
static std::mutex m_ChatInputMutex;

CChat::CChat()
{
	CNecessary::Log(OBFUSCATE("CChat Initialise"));

	CChat::m_dwTextColor = 0xFFFFFFFF;
	CChat::m_dwInfoColor = 0x62AA88FF;
	CChat::m_dwDebugColor = 0xBEBEBEFF;

	CCommand::Initialise();
}

CChat::~CChat()
{
    {
        std::lock_guard<std::mutex> lock(m_ChatInputMutex);
        m_ChatInputQueue = {};
    }
    MGCloseFlutterChat();
}

bool CChat::QueueInput(std::string input)
{
    if (input.empty() || input.size() > eMaxItem::MAX_MESSAGE_LENGTH || input.find('\0') != std::string::npos) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_ChatInputMutex);
    if (m_ChatInputQueue.size() >= 16) {
        return false;
    }
    m_ChatInputQueue.push(std::move(input));
    return true;
}

void CChat::ProcessPendingChatInputs()
{
    std::queue<std::string> localQueue;

    {
        std::lock_guard<std::mutex> lock(m_ChatInputMutex);
        localQueue.swap(m_ChatInputQueue);
    }

    while (!localQueue.empty())
    {
        std::string input = std::move(localQueue.front());
        localQueue.pop();

        if (CGui::GetChat())
        {
            CGui::GetChat()->ChatWindowInputHandler(input.c_str());
        }
    }
}

void CChat::ChatWindowInputHandler(const char* str)
{
	if (!str || *str == '\0') {
		return;
	}

	if (!CMultiPlayer::GetActiveNetwork()) {
		return;
	}

	if (*str == '/')
	{
		if (CCommand::Process(str)) {
			return;
		}

		if (!CMultiPlayerPacket::SendChatCommand(str)) {
            CChat::AddInfoMessage(OBFUSCATE("Chat is unavailable until the server connection is ready."));
        }
	}
	else
	{
		if (!CMultiPlayerPacket::SendChatMessage(str)) {
            CChat::AddInfoMessage(OBFUSCATE("Chat is unavailable until the server connection is ready."));
        }
	}
}

void CChat::Render()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    // if (CGui::GetScoreBoard()->GetActiveScore() || CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() ||
    //     CGui::GetVoiceMenu()->GetActiveVoiceMenu() || CVoiceMenu::m_bCustomizeMode /*|| CGui::GetClientMenu()->GetActiveClientMenu()*/)
    // {
    //     MGSetFlutterChatVisible(false);
    //     return;
    // }

    MGSetFlutterChatVisible(true);

    if (CPad::IsActiveWidgets() || CGame::IsRadarActive()
        // || CRadar::GetAlpha() <= 0
        // || CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsSpectating()
    )
    {
        MGSetFlutterChatPosition(0);
    }
    else if (auto* settings = CSetting::GetConfig())
    {
        const float position = settings->GetFloatValue(OBFUSCATE("JavaChatPosX"));
        MGSetFlutterChatPosition(std::isfinite(position) ? static_cast<int>(std::clamp(position, 0.0f, 16.0f)) : 16);
    }
}

void CChat::AddChatMessage(const char* szNick, uint32_t dwNickColor, const char* szMessage)
{
	CChat::AddToChatWindowBuffer(eChatMessageType::CHAT_TYPE_CHAT, szMessage, szNick, CChat::m_dwTextColor, dwNickColor, 0);
}

void CChat::AddInfoMessage(const char* szFormat, ...)
{
    std::string message;

    va_list args;
    va_start(args, szFormat);

    va_list args_copy;
    va_copy(args_copy, args);

    int len = vsnprintf(nullptr, 0, szFormat, args);
    if (len >= 0)
    {
        message.resize(static_cast<size_t>(len) + 1);
        vsnprintf(message.data(), message.size(), szFormat, args_copy);
    }

    va_end(args_copy);
    va_end(args);

    CChat::AddToChatWindowBuffer(eChatMessageType::CHAT_TYPE_INFO, message.c_str(), nullptr, CChat::m_dwInfoColor, 0, 0);
}

void CChat::AddDebugMessage(int alerttype, const char* szFormat, ...)
{
    std::string message;

    va_list args;
    va_start(args, szFormat);

    va_list args_copy;
    va_copy(args_copy, args);

    int len = vsnprintf(nullptr, 0, szFormat, args);
    if (len >= 0)
    {
        message.resize(static_cast<size_t>(len) + 1);
        vsnprintf(message.data(), message.size(), szFormat, args_copy);
    }

    va_end(args_copy);
    va_end(args);

    CChat::AddToChatWindowBuffer(eChatMessageType::CHAT_TYPE_DEBUG, message.c_str(), nullptr, CChat::m_dwDebugColor, 0, alerttype);
}

void CChat::AddClientMessage(uint32_t dwColor, const char* szStr)
{
	CChat::AddToChatWindowBuffer(eChatMessageType::CHAT_TYPE_INFO, szStr, nullptr, dwColor, 0, 0);
}

void CChat::AddToChatWindowBuffer(eChatMessageType type, const char* szString, const char* szNick, uint32_t dwTextColor, uint32_t dwNickColor, int alerttype)
{
    if (!szString || !*szString) {
        return;
    }

    CHAT_WINDOW_ENTRY entry{};
    entry.eType = type;
    entry.dwNickColor = dwNickColor | 0x000000FF;
    entry.dwTextColor = dwTextColor | 0x000000FF;
    if (szNick) {
        std::snprintf(entry.szNick, sizeof(entry.szNick), "%s", szNick);
    }
    std::snprintf(entry.szMessage, sizeof(entry.szMessage), "%s", szString);
    CChat::FilterInvalidChars(entry.szMessage);
    CChat::FilterInvalidChars(entry.szNick);

    MGAddFlutterChatMessage(entry.eType == CHAT_TYPE_DEBUG ? alerttype : 0, entry.dwTextColor, entry.dwNickColor, entry.szNick, entry.szMessage);
}

void CChat::FilterInvalidChars(char *szString)
{
	if (szString == NULL) {
		return;
	}

	while (*szString)
	{
		if(*szString > 0 && *szString < ' ')
			*szString = ' ';

		szString++;
	}
}
