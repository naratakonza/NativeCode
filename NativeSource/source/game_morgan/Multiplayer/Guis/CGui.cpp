#include "../../../../Main.h"
#include "CGui.h"

CDebugInfo* CGui::pDebug = nullptr;
CFontRender* CGui::pFontRender = nullptr;

void CGui::Initialise()
{
    CGui::pDebug = new CDebugInfo();
}

void CGui::InitialiseFont()
{
    if (!CGui::pFontRender) {
        CGui::pFontRender = new CFontRender();
    }
}

void CGui::Uninitialise()
{
    SAFE_DELETE(CGui::pDebug);
    SAFE_DELETE(CGui::pFontRender);
}
