#pragma once

#define DIALOG_STYLE_MSGBOX		0
#define DIALOG_STYLE_INPUT		1
#define DIALOG_STYLE_LIST		2
#define DIALOG_STYLE_PASSWORD	3
#define DIALOG_STYLE_TABLIST	4
#define DIALOG_STYLE_TABLIST_HEADERS	5

class CDialog
{
public:
	CDialog();
	~CDialog();

	void Show();
	void Close();
	void SetInfo(const char* szInfo, int length);
	void Enable(bool Enable);
	void ProcessPendingResponse();

	static constexpr int DIALOG_BUFFER_SIZE = 100;
	
	bool GetActiveDialog() {
		return CDialog::m_bHasActiveDialog;
	}

	static bool DialogWindowInputHandler(uint16_t dialogId, uint8_t buttonId, uint16_t listItemId, const char* str);

public:
	bool		m_bHasActiveDialog;
	uint8_t 	m_byteDialogStyle;
	uint16_t	m_wDialogID;
	char		m_utf8Title[256 + 1];
	char*       m_putf8Info;
	char*       m_pszInfo;
	char		m_utf8Button1[256 + 1];
	char		m_utf8Button2[256 + 1];

private:
	bool		m_hasPendingResponse = false;
	uint8_t		m_pendingButton = 0;
	uint16_t	m_pendingListItem = 0;
	char		m_pendingInput[DIALOG_BUFFER_SIZE]{};
};
