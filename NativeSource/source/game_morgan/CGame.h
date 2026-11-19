#pragma once

#include "CCamera.h"
#include "CRect.h"
#include "Entity/Ped/CPlayerPed.h"
#include "Multiplayer/Snapshot/CModelSnapshot.h"

class CGame
{
public:
	static CCamera* m_pGameCamera;
	static CPlayerPed* m_pGamePlayer;
    static CModelSnapshot* m_pModelSnapshot;

public:
    static void Initialize();
    static void Uninitialise();
    static void SetFramerate(std::uint8_t fps);
    static void ToggleRadar(bool iToggle);
    static uint8_t IsMenuActive();
    static bool IsRadarActive();
    static void DisplayHUD(bool bDisp);
    static void GetScreenWidthAndHeight(int* w, int* h);
    static CRect GetScreenSafeArea();
    static bool IsAnimationLoaded(const char* animation);
    static void RequestAnimation(const char* animation);
    static void LoadAnimationsAndModels();
    static std::uint8_t GetActiveInterior();
    static bool IsDrawingBigMap();
    static void MenuSwitchOff();
    static void RefreshStreamingAt(CVector pos);
	static int GetPlayerMoney();
	static void AddPlayerMoney(int money);
	static void ResetPlayerMoney();
    static void SetPlayerMaxStats();
    static void SetPlayerStatsLocked(bool locked);
    static float FindGroundZForCoord(float x, float y, float z);
    static void ToggleWidescreen(uint8_t Toggle);
	static void CreateExplosion(float X, float Y, float Z, int iType, float Radius);

public:
	static CCamera* GetCamera() {
		if (!m_pGameCamera) m_pGameCamera = new CCamera();
		return m_pGameCamera;
	};

	static CPlayerPed* GetPlayer() {
		if (!m_pGamePlayer) m_pGamePlayer = new CPlayerPed();
		return m_pGamePlayer;
	};

	static CModelSnapshot* GetModelSnapshot() {
		if (!m_pModelSnapshot) m_pModelSnapshot = new CModelSnapshot();
		return m_pModelSnapshot;
	};
};