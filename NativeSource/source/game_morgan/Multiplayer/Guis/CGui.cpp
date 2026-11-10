#include "../../../../Main.h"
#include "CGui.h"

CDebugInfo* CGui::pDebug = nullptr;
CFontRender* CGui::pFontRender = nullptr;

void CGui::Initialise()
{
    CNecessary::Log(OBFUSCATE("CGui Initialise"));

    CGui::pDebug = new CDebugInfo();
}

void CGui::InitialiseFont()
{
    CGui::pFontRender = new CFontRender();
}

void CGui::Uninitialise()
{
    CNecessary::Log(OBFUSCATE("CGui Uninitialise"));

    SAFE_DELETE(CGui::pDebug);
    SAFE_DELETE(CGui::pFontRender);
}