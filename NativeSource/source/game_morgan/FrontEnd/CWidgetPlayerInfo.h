#pragma once

#include <cstdint>

class CWidgetPlayerInfo
{
public:
    static void Initialise();
    static void RenderArmorBar(uintptr_t thiz, int Player, float fTop);
};