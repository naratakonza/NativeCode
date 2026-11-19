#pragma once

#include "../Texts/CDebugInfo.h"
#include "../Texts/CPlayerTags.h"
#include "CChat.h"
#include "CButton.h"
#include "CDialog.h"
#include "CXSing.h"
#include "CSpawnScreen.h"
#include "Font/CFontRender.h"
#include "Gps/CGPSMarker.h"
#include "Gps/CGPSNavigator.h"

class CGui
{
public:
    static void Initialise();
    static void InitialiseFont();
    static void Uninitialise();

    static CPlayerTags* GetPlayerTag()
    {
        return CGui::pPlayerTag;
    }

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

    static CGPSMarker* GetGPSMarker()
    {
        return CGui::pGPSMarker;
    }

    static CGPSNavigator* GetGPSNavigator()
    {
        return CGui::pGPSNavigator;
    }

    static CDebugInfo* GetDebug() {
        return pDebug;
    }

    static CXSing* GetXSing()
    {
        return CGui::pXSing;
    }

    static CFontRender* GetFontRender() {
        return pFontRender;
    }

private:
    static CPlayerTags* pPlayerTag;
    static CChat* pChat;
    static CButton* pButton;
    static CDialog* pDialog;
    static CSpawnScreen* pSpawnScreen;
    static CGPSMarker* pGPSMarker;
    static CGPSNavigator* pGPSNavigator;
    static CDebugInfo* pDebug;
    static CXSing* pXSing;
    static CFontRender* pFontRender;
};