#include "../../../../Main.h"
#include "CGui.h"

CScoreBoard* CGui::pScoreBoard = nullptr;
CDeathWindow* CGui::pDeathWindow = nullptr;
CPlayerTags* CGui::pPlayerTag = nullptr;
CDebugInfo* CGui::pDebug = nullptr;
CXSing* CGui::pXSing = nullptr;
CChat* CGui::pChat = nullptr;
CButton* CGui::pButton = nullptr;
CDialog* CGui::pDialog = nullptr;
CSpawnScreen* CGui::pSpawnScreen = nullptr;
CGPSMarker* CGui::pGPSMarker = nullptr;
CGPSNavigator* CGui::pGPSNavigator = nullptr;
CFontRender* CGui::pFontRender = nullptr;

void CGui::Initialise()
{
    CNecessary::Log(OBFUSCATE("CGui Initialise"));

    CGui::pScoreBoard = new CScoreBoard();
    CGui::pDeathWindow = new CDeathWindow();
    CGui::pPlayerTag = new CPlayerTags();
    CGui::pDebug = new CDebugInfo();
    CGui::pXSing = new CXSing();
    CGui::pChat = new CChat();
    CGui::pButton = new CButton();
    CGui::pDialog = new CDialog();
    CGui::pSpawnScreen = new CSpawnScreen();
    CGui::pGPSMarker = new CGPSMarker();
    CGui::pGPSNavigator = new CGPSNavigator();
}

void CGui::InitialiseFont()
{
    CGui::pFontRender = new CFontRender();
}

void CGui::Uninitialise()
{
    CNecessary::Log(OBFUSCATE("CGui Uninitialise"));

    SAFE_DELETE(CGui::pScoreBoard);
    SAFE_DELETE(CGui::pDeathWindow);
    SAFE_DELETE(CGui::pPlayerTag);
    SAFE_DELETE(CGui::pDebug);
    SAFE_DELETE(CGui::pXSing);
    SAFE_DELETE(CGui::pSpawnScreen);
    SAFE_DELETE(CGui::pGPSMarker);
    SAFE_DELETE(CGui::pGPSNavigator);
    SAFE_DELETE(CGui::pButton);
    SAFE_DELETE(CGui::pChat);
    SAFE_DELETE(CGui::pDialog);
    SAFE_DELETE(CGui::pFontRender);
}