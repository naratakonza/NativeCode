#pragma once

#include "CCamera.h"

class CGame
{
public:
	static CCamera* m_pGameCamera;

public:
    static void Initialize();
    static void Uninitialise();
    static void SetFramerate(std::uint8_t fps);
    static void ToggleRadar(bool iToggle);
    static bool IsRadarActive();
    static void DisplayHUD(bool bDisp);
    static bool IsAnimationLoaded(const char* animation);
    static void RequestAnimation(const char* animation);
    static void LoadAnimationsAndModels();
    static std::uint8_t GetActiveInterior();
    static bool IsDrawingBigMap();
    static void MenuSwitchOff();
    static void SetPlayerMaxStats();
    static void SetPlayerStatsLocked(bool locked);

public:
	static CCamera* GetCamera() {
		if (!m_pGameCamera) m_pGameCamera = new CCamera();
		return m_pGameCamera;
	};
};
