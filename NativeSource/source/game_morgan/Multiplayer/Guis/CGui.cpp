#include "../../../../Main.h"
#include "CGui.h"

CDebugInfo* CGui::pDebug = nullptr;
CChat* CGui::pChat = nullptr;
CFontRender* CGui::pFontRender = nullptr;

void CGui::Initialise()
{
    CNecessary::Log(OBFUSCATE("CGui Initialise"));

    CGui::pDebug = new CDebugInfo();
    CGui::pChat = new CChat();
}

void CGui::InitialiseFont()
{
    CGui::pFontRender = new CFontRender();
}

void CGui::Uninitialise()
{
    CNecessary::Log(OBFUSCATE("CGui Uninitialise"));

    SAFE_DELETE(CGui::pDebug);
    SAFE_DELETE(CGui::pChat);
    SAFE_DELETE(CGui::pFontRender);
}
