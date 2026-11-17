#pragma once

#include "../Texts/CDebugInfo.h"
#include "CChat.h"
#include "CButton.h"
#include "CDialog.h"
#include "CSpawnScreen.h"
#include "Font/CFontRender.h"

class CGui
{
public:
    static void Initialise();
    static void InitialiseFont();
    static void Uninitialise();

    static CChat* GetChat()
    {
        return CGui::pChat;
    }

    static CDialog* GetDialog()
    {
		return CGui::pDialog;
	};

    static CButton* GetButton()
    {
        return CGui::pButton;
    }

    static CSpawnScreen* GetSpawnScreen()
    {
        return CGui::pSpawnScreen;
    }

    static CDebugInfo* GetDebug() {
        return pDebug;
    }

    static CFontRender* GetFontRender() {
        return pFontRender;
    }

private:
    static CChat* pChat;
    static CButton* pButton;
    static CDialog* pDialog;
    static CSpawnScreen* pSpawnScreen;
    static CDebugInfo* pDebug;
    static CFontRender* pFontRender;
};