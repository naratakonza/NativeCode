#pragma once

#include "../Texts/CDebugInfo.h"
#include "Font/CFontRender.h"

class CGui
{
public:
    static void Initialise();
    static void Uninitialise();
    static CDebugInfo* GetDebug();
    static CFontRender* GetFontRender();

private:
    static CDebugInfo* pDebug;
    static CFontRender* pFontRender;
};
