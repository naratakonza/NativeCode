#pragma once

#include "../Guis/Font/CFontRender.h"
#include "../../Enums/eMaxItem.h"
#include "../Syncs/CRemotePlayer.h"

class CPlayerTags
{
public:
	static bool m_bHealthBarStyle;
	static bool m_bWeaponTag;

public:
	CPlayerTags();
	~CPlayerTags();

	void Render();
	void DrawComponentsDefault(CRemotePlayer* pPlayer, CVector Out, char* tempBuff);
	void DrawComponentsNew(CRemotePlayer* pPlayer, CVector Out, char* tempBuff);

	const char* GetWeaponSpriteId(uint8_t nWeapon);
	static void SetStyleHealthBar(bool bEnabled);
	static void SetWeaponTag(bool bEnabled);

private:
	ImVec2 HealthBarBDR1;
	ImVec2 HealthBarBDR2;
	ImVec2 HealthBarBG1;
	ImVec2 HealthBarBG2;
	ImVec2 HealthBar1;
	ImVec2 HealthBar2;

	ImVec2 ArmourBarBG1;
	ImVec2 ArmourBarBG2;
	ImVec2 ArmourBar1;
	ImVec2 ArmourBar2;

	ImColor HealthBarColor;
	ImColor HealthBarBGColor;
	ImColor HealthBarBDRColor;
};