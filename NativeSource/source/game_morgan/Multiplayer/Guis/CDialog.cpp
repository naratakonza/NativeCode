#include "../../../../Main.h"
#include "CGui.h"
#include "CDialog.h"
#include "../../../../Launcher/MGFlutter.h"
#include "../CMultiPlayer.h"
#include "../Core/CMultiPlayerPacket.h"

CDialog::CDialog()
{
	CDialog::m_bHasActiveDialog = false;
	CDialog::m_byteDialogStyle = DIALOG_STYLE_MSGBOX;
	CDialog::m_wDialogID = eInvalidId::INVALID_ID;
    CDialog::m_putf8Info = nullptr;
	CDialog::m_pszInfo = nullptr;
}

CDialog::~CDialog()
{
    CDialog::Close();
}

void CDialog::Show()
{
	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CGui::GetScoreBoard() && CGui::GetScoreBoard()->GetActiveScore())
		{
            CGui::GetScoreBoard()->Close();
        }

        // ยังไม่ใช้ในตอนนี้
		// if (CGui::GetMainMenuScreens()->GetActiveMainMenuScreens())
		// {
        //     CGui::GetMainMenuScreens()->Close();
        // }

        // ยังไม่ใช้ในตอนนี้
        // HideSettingsMenu();

        CDialog::Enable(true);
        MGShowFlutterDialog(CDialog::m_wDialogID, CDialog::m_byteDialogStyle, CDialog::m_utf8Title, CDialog::m_putf8Info, CDialog::m_utf8Button1, CDialog::m_utf8Button2);
	}
}

void CDialog::Close()
{
	if (CDialog::m_putf8Info)
	{
		free(CDialog::m_putf8Info);
		CDialog::m_putf8Info = nullptr;
	}

	if (CDialog::m_pszInfo)
	{
		free(CDialog::m_pszInfo);
		CDialog::m_pszInfo = nullptr;
	}

	CDialog::m_bHasActiveDialog = false;
	CDialog::m_byteDialogStyle = DIALOG_STYLE_MSGBOX;
	CDialog::m_wDialogID = eInvalidId::INVALID_ID;
	memset(CDialog::m_utf8Title,   0, sizeof(CDialog::m_utf8Title));
	memset(CDialog::m_utf8Button1, 0, sizeof(CDialog::m_utf8Button1));
	memset(CDialog::m_utf8Button2, 0, sizeof(CDialog::m_utf8Button2));

    CDialog::Enable(false);
}

void CDialog::SetInfo(const char* szInfo, int length)
{
   	if (!szInfo || length <= 0) {
        return;
    }

	if (CMultiPlayer::GetActiveNetwork())
	{
		if (CDialog::m_putf8Info)
        {
            free(CDialog::m_putf8Info);
            CDialog::m_putf8Info = nullptr;
        }

        if (CDialog::m_pszInfo)
        {
            free(CDialog::m_pszInfo);
            CDialog::m_pszInfo = nullptr;
        }

		CDialog::m_putf8Info = (char*)malloc((length * 3) + 1);
		if (!CDialog::m_putf8Info) {
			CNecessary::Log(OBFUSCATE("CDialog::SetInfo: can't allocate memory"));
			return;
		}

		CDialog::m_pszInfo = (char*)malloc((length * 3) + 1);
        if (!CDialog::m_pszInfo)
        {
            CNecessary::Log(OBFUSCATE("CDialog::SetInfo: can't allocate memory for m_pszInfo"));
            free(CDialog::m_putf8Info);
            CDialog::m_putf8Info = nullptr;
			
            return;
        }

		CNecessary::ConvertLocalizedEncodingToUTF8(CDialog::m_putf8Info, szInfo, (length * 3) + 1);

		strncpy(CDialog::m_pszInfo, CDialog::m_putf8Info, (length * 3) + 1);
        CDialog::m_pszInfo[(length * 3)] = '\0';
	}
}

void CDialog::Enable(bool Enable)
{
	CDialog::m_bHasActiveDialog = Enable;

    if (!Enable)
    {
        CDialog::m_hasPendingResponse = false;
        CDialog::m_pendingInput[0] = '\0';
        MGHideFlutterDialog();
    }
}

bool CDialog::DialogWindowInputHandler(uint16_t dialogId, uint8_t buttonId, uint16_t listItemId, const char* str)
{
    CDialog* dialog = CGui::GetDialog();
    if (CGame::IsMenuActive() || !dialog || !dialog->GetActiveDialog() || dialogId != dialog->m_wDialogID || buttonId > 1 || dialog->m_hasPendingResponse) {
        return false;
    }

    dialog->m_pendingButton = buttonId;
    dialog->m_pendingListItem = listItemId;
    snprintf(dialog->m_pendingInput, sizeof(dialog->m_pendingInput), OBFUSCATE("%s"), str ? str : "");
    dialog->m_hasPendingResponse = true;
    return true;
}

void CDialog::ProcessPendingResponse()
{
    if (!CDialog::m_hasPendingResponse || CGame::IsMenuActive()) {
        return;
    }

    CMultiPlayerPacket::SendDialogResponse(CDialog::m_wDialogID, CDialog::m_pendingButton, CDialog::m_pendingListItem, CDialog::m_pendingInput);
    CDialog::Close();
}