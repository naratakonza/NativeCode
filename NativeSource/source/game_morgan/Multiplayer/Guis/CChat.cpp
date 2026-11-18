#include "../../../../Main.h"
#include "CChat.h"
#include "CGui.h"
#include "CCommand.h"
#include "../CMultiPlayer.h"
#include "../Core/CMultiPlayerPacket.h"
#include "../../Base/CPad.h"
#include "../../CUtil.h"
#include "../../CRadar.h"
#include "../../RadarMod/CTiltedRadar.h"
#include "../../../../Launcher/MGFlutter.h"
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

		CMultiPlayerPacket::SendChatCommand(str);
	}
	else
	{
		CMultiPlayerPacket::SendChatMessage(str);
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

    const bool radarHidden = CGame::IsRadarActive();
    const float radarRight = radarHidden ? -1.0f : CTiltedRadar::GetWidgetRightFraction();

    if (CPad::IsActiveWidgets() || radarHidden || CRadar::GetAlpha() <= 0 || CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsSpectating())
    {
        MGSetFlutterChatPosition(0, radarRight);
    }
    else
    {
        auto* settings = CSetting::GetConfig();
        const float position = settings ? settings->GetFloatValue(OBFUSCATE("JavaChatPosX")) : 16.0f;
        MGSetFlutterChatPosition(std::isfinite(position) ? static_cast<int>(std::clamp(position, 0.0f, 16.0f)) : 16, radarRight);
    }

    MGSetFlutterChatVisible(true);
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
    std::string strString = CUtil::ReplaceKey(std::string(szString));
    CChat::FilterInvalidChars(strString.data());
    entry.eType = type;
    entry.dwNickColor = dwNickColor | 0x000000FF;
    entry.dwTextColor = dwTextColor | 0x000000FF;

    if (szNick)
    {
        std::snprintf(entry.szNick, sizeof(entry.szNick), "%s", szNick);
    }

    CChat::FilterInvalidChars(entry.szNick);
    CNecessary::ConvertLocalizedEncodingToUTF8(entry.utf8Message, strString.c_str(), sizeof(entry.utf8Message));
    char utf8Nick[eMaxItem::MAX_PLAYER_NAME * 3 + 1]{};
    CNecessary::ConvertLocalizedEncodingToUTF8(utf8Nick, entry.szNick, sizeof(utf8Nick));

    MGAddFlutterChatMessage(entry.eType == eChatMessageType::CHAT_TYPE_DEBUG ? alerttype : 0, entry.dwTextColor, entry.dwNickColor, utf8Nick, entry.utf8Message);
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