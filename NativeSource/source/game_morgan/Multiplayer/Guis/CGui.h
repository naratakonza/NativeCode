#pragma once

#include "../Texts/CDebugInfo.h"
#include "CChat.h"
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

    static CDebugInfo* GetDebug() {
        return pDebug;
    }

    static CFontRender* GetFontRender() {
        return pFontRender;
    }

private:
    static CChat* pChat;
    static CDebugInfo* pDebug;
    static CFontRender* pFontRender;
};
