#pragma once

#include "../../Enums/eChatMessageType.h"
#include "../../Enums/eMaxItem.h"

typedef struct
{
    eChatMessageType	eType;
    char 				utf8Message[eMaxItem::MAX_MESSAGE_LENGTH * 3 + 1];
    char 				szNick[eMaxItem::MAX_PLAYER_NAME + 1];
    uint32_t			dwTextColor;
    uint32_t 			dwNickColor;
} CHAT_WINDOW_ENTRY;

class CChat
{
	friend class CGui;
public:
	CChat();
	~CChat();

	void Render();
	static bool QueueInput(std::string input);
	void ProcessPendingChatInputs();
	void ChatWindowInputHandler(const char* str);
	void AddChatMessage(const char* szNick, uint32_t dwNickColor, const char* szMessage);
	void AddInfoMessage(const char* szFormat, ...);
	void AddDebugMessage(int alerttype, const char* szFormat, ...);
	void AddClientMessage(uint32_t dwColor, const char* szStr);

private:
	void FilterInvalidChars(char* szString);
	void AddToChatWindowBuffer(eChatMessageType eType, const char* szString, const char* szNick, uint32_t dwTextColor, uint32_t dwNickColor, int alerttype);

private:
	uint32_t m_dwTextColor;
	uint32_t m_dwInfoColor;
	uint32_t m_dwDebugColor;
};