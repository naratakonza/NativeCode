#pragma once

#include "../Enums/eWidgetState.h"
#include "../Enums/eWidgetType.h"

class CWidgets
{
public:
	static void Initialise();
    static eWidgetState ProcessWidget(uintptr_t pWidget);
    static eWidgetType GetWidgetTypeFromWidget(uintptr_t pWidget);
    static void ManageAlpha(uintptr_t thiz);
    static void SetAlpha(int Alpha);
	static float GetAlpha();
    static void SetToggle(bool iToggle);
    static bool IsToggleActive();

public:
    static int m_iAlpha;
    static bool m_bToggleWidget;
};