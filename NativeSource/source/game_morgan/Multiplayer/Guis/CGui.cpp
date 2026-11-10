#include "CGui.h"

CDebugInfo* CGui::pDebug = nullptr;
CFontRender* CGui::pFontRender = nullptr;

void CGui::Initialise()
{
    if (!CGui::pDebug) {
        CGui::pDebug = new CDebugInfo();
    }
    if (!CGui::pFontRender) {
        CGui::pFontRender = new CFontRender();
    }
}

void CGui::Uninitialise()
{
    delete CGui::pFontRender;
    CGui::pFontRender = nullptr;
    delete CGui::pDebug;
    CGui::pDebug = nullptr;
}

CDebugInfo* CGui::GetDebug()
{
    return CGui::pDebug;
}

CFontRender* CGui::GetFontRender()
{
    return CGui::pFontRender;
}
