#pragma once

#include "../Texts/CDebugInfo.h"
#include "Font/CFontRender.h"

class CGui
{
public:
    static void Initialise();
    static void InitialiseFont();
    static void Uninitialise();

    static CDebugInfo* GetDebug() {
        return pDebug;
    }

    static CFontRender* GetFontRender() {
        return pFontRender;
    }

private:
    static CDebugInfo* pDebug;
    static CFontRender* pFontRender;
};