#include "../../../Main.h"
#include "CWidgets.h"
#include "../CHud.h"
#include "../Multiplayer/CMultiPlayer.h"
#include "../Multiplayer/Guis/CGui.h"

#include "../Enums/eMaxConfig.h"
#include "../Enums/eWidgetIDs.h"

#include <mach-o/dyld.h>

int CWidgets::m_iAlpha = 255;
bool CWidgets::m_bToggleWidget = true;

static bool widgetsHooksInstalled = false;

extern "C" __attribute__((visibility("default"))) void CWidgetButton_Draw_hook(uintptr_t thiz, void (*original)(uintptr_t))
{
    if (!thiz) {
        return;
    }

    if (!widgetsHooksInstalled || !CMultiPlayer::IsSampActive())
    {
        original(thiz);
        return;
    }

	if (!CMultiPlayer::GetActiveNetwork()) {
		return;
	}

	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->IsSpectating()) {
		return;
	}

	// if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()) 
    // {
	// 	if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
	// 	{
	// 		return;
	// 	}
	// }

	if (!CWidgets::IsToggleActive()) {
		return;
	}

	if (CGui::GetDialog() /*&& CGui::GetScoreBoard() && CGui::GetVoiceMenu() && CGui::GetMainMenuScreens()*/)
	{
		if (CGui::GetDialog()->GetActiveDialog() /*|| CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() || CGui::GetScoreBoard()->GetActiveScore() || (CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CVoiceMenu::m_bCustomizeMode)*/)
		{
			return;
		}
	}

	original(thiz);
}

extern "C" __attribute__((visibility("default"))) void CWidget_Update_hook(uintptr_t pWidget, void (*original)(uintptr_t))
{
    if (widgetsHooksInstalled && CMultiPlayer::GetActiveNetwork() && CWidgets::ProcessWidget(pWidget) == eWidgetState::STATE_FIXED) {
        return;
    }

    original(pWidget);
}

extern "C" __attribute__((visibility("default"))) void CWidget_SetEnabled_hook(uintptr_t pWidget, bool bEnabled, void (*original)(uintptr_t, bool))
{
    if (bEnabled && widgetsHooksInstalled && CMultiPlayer::GetActiveNetwork() && CWidgets::ProcessWidget(pWidget) == eWidgetState::STATE_FIXED)
    {
        bEnabled = false;
    }

    original(pWidget, bEnabled);
}

extern "C" __attribute__((visibility("default"))) void CWidget_ManageAlpha_hook(uintptr_t thiz, void (*original)(uintptr_t))
{
    if (widgetsHooksInstalled && CMultiPlayer::IsSampActive())
    {
        CWidgets::ManageAlpha(thiz);
        return;
    }

    original(thiz);
}

void CWidgets::Initialise()
{
    CNecessary::Log(OBFUSCATE("CWidgets Initialise"));

    const bool draw = CMemory::HasMappedInstruction(SA_ADDR(0x1FF208), "\x0F\x61\x0A\x14") && CMemory::HasMappedBytes(SA_ADDR(0x497644), "\x81\x00\x00\x10\xF0\x00\x00\xD0\x10\x86\x42\xF9\x00\x02\x1F\xD6\xFF\x83\x01\xD1\xED\x9E\xF5\x17", 24, "__mg_gameproc");
    const bool update = CMemory::HasMappedInstruction(SA_ADDR(0x1FD8A0), "\x6F\x67\x0A\x14") && CMemory::HasMappedBytes(SA_ADDR(0x49765C), "\x81\x00\x00\x10\xF0\x00\x00\xD0\x10\x8A\x42\xF9\x00\x02\x1F\xD6\xFF\x43\x01\xD1\x8D\x98\xF5\x17", 24, "__mg_gameproc");
    const bool setEnabled = CMemory::HasMappedInstruction(SA_ADDR(0x1FEBAC), "\xB2\x62\x0A\x14") && CMemory::HasMappedBytes(SA_ADDR(0x497674), "\x82\x00\x00\x10\xF0\x00\x00\xD0\x10\x8E\x42\xF9\x00\x02\x1F\xD6\x01\x64\x01\x39\x4A\x9D\xF5\x17", 24, "__mg_gameproc");
    const bool manageAlpha = CMemory::HasMappedInstruction(SA_ADDR(0x1FECC0), "\x73\x62\x0A\x14") && CMemory::HasMappedBytes(SA_ADDR(0x49768C), "\x81\x00\x00\x10\xF0\x00\x00\xD0\x10\x92\x42\xF9\x00\x02\x1F\xD6\x08\x64\x41\x39\x89\x9D\xF5\x17", 24, "__mg_gameproc");
    const bool widgetTable = CMemory::HasMappedBytes(SA_ADDR(0x1F033C), "\x28\x2D\x00\x90\x08\x01\x35\x91\x08\x85\x42\xF9", 12, "__text");
    const bool playerInfo = CMemory::HasMappedBytes(SA_ADDR(0x208670), "\xC8\x14\x00\xF0\x08\x41\x3A\x91\x08\x00\x00\xF9", 12, "__text");
    const bool timing = CMemory::HasMappedBytes(SA_ADDR(0x1FECC4), "\x00\x60\x41\x3D\x00\xD8\x61\x7E\xE9\x2C\x00\xB0\x21\x99\x45\xFD\x02\x28\x40\xBD", 20, "__text");

    widgetsHooksInstalled = draw && update && setEnabled && manageAlpha && widgetTable && playerInfo && timing;
    if (!widgetsHooksInstalled)
    {
        CNecessary::Log(OBFUSCATE("CWidgets iOS bridges unavailable"));
    }
}

void CWidgets::ManageAlpha(uintptr_t thiz)
{
    if (!thiz || !widgetsHooksInstalled) {
        return;
    }

    auto& alpha = *reinterpret_cast<uint8_t*>(thiz + 0x58);
    const float alphaChangeSpeed = *reinterpret_cast<const float*>(thiz + 0x28);
    static const auto* elapsedTime = reinterpret_cast<const double*>(SA_ADDR(0x79BB30) + _dyld_get_image_vmaddr_slide(0));
    const float change = static_cast<float>(*elapsedTime) * alphaChangeSpeed;
    if (!std::isfinite(change) || change < 0.0f) {
        return;
    }

    float currentAlpha = static_cast<float>(alpha);
    if (*reinterpret_cast<const uint8_t*>(thiz + 0x59))
    {
        currentAlpha += change;

        float maxAlpha = CWidgets::GetAlpha();
        const auto widgetType = CWidgets::GetWidgetTypeFromWidget(thiz);
        // if (widgetType == eWidgetType::TYPE_MAP)
        // {
        //     maxAlpha = CRadar::GetAlpha();
        // }
        if (widgetType == eWidgetType::TYPE_PLAYER_INFO)
        {
            maxAlpha = CHud::GetAlpha();
        }

        currentAlpha = std::fmin(currentAlpha, maxAlpha);
    }
    else
    {
        currentAlpha = std::fmax(currentAlpha - change, 0.0f);
    }

    alpha = static_cast<uint8_t>(std::round(currentAlpha));
}

eWidgetState CWidgets::ProcessWidget(uintptr_t pWidget)
{
	eWidgetType widgetType = CWidgets::GetWidgetTypeFromWidget(pWidget);

	if (CGame::GetPlayer())
	{
		switch (widgetType) {
			case eWidgetType::TYPE_NONE: {
				return eWidgetState::STATE_NONE;

				break;
			}

			case eWidgetType::TYPE_DIVE: {
				if (!CGame::GetPlayer()->IsInWater())
				{
					return eWidgetState::STATE_FIXED;
				}
				// else
				// {
					// if (CGame::GetPlayer()->IsInJetpack())
					// {
						// return eWidgetState::STATE_FIXED;
					// }
				// }

				break;
			}

			case eWidgetType::TYPE_SWIM: {
				if (!CGame::GetPlayer()->IsInWater())
				{
					return eWidgetState::STATE_FIXED;
				}
				// else
				// {
					// if (CGame::GetPlayer()->IsInJetpack())
					// {
						// return eWidgetState::STATE_FIXED;
					// }
				// }

				break;
			}

			case eWidgetType::TYPE_ACCELERATE: {
				if (!CGame::GetPlayer()->IsInVehicle() /* && !CGame::GetPlayer()->IsInJetpack() */)
				{
					return eWidgetState::STATE_FIXED;
				}

				break;
			}

			case eWidgetType::TYPE_BRAKE: {
				if (!CGame::GetPlayer()->IsInVehicle() /* && !CGame::GetPlayer()->IsInJetpack() */)
				{
					return eWidgetState::STATE_FIXED;
				}

				break;
			}

			case eWidgetType::TYPE_FIRE_LEFT: {
				if (!CGame::GetPlayer()->IsInVehicle() /* && !CGame::GetPlayer()->IsInJetpack() */)
				{
					return eWidgetState::STATE_FIXED;
				}

				break;
			}

			case eWidgetType::TYPE_FIRE_RIGHT: {
				if (!CGame::GetPlayer()->IsInVehicle() /* && !CGame::GetPlayer()->IsInJetpack() */)
				{
					return eWidgetState::STATE_FIXED;
				}

				break;
			}
		}
	}

	return eWidgetState::STATE_NONE;
}

eWidgetType CWidgets::GetWidgetTypeFromWidget(uintptr_t pWidget)
{
    if (!pWidget || !widgetsHooksInstalled) {
        return eWidgetType::TYPE_NONE;
    }

    static const auto slide = _dyld_get_image_vmaddr_slide(0);
    if (*reinterpret_cast<const uintptr_t*>(pWidget) == SA_ADDR(0x4A3E90) + slide) {
        return eWidgetType::TYPE_PLAYER_INFO;
    }

    static const auto* widgets = reinterpret_cast<const uintptr_t*>(SA_ADDR(0x794D40) + slide);
    if (pWidget == widgets[eWidgetIDs::WIDGET_ATTACK]) {
        return eWidgetType::TYPE_PUNCH;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_BUTTON_SPRINT]) {
        return eWidgetType::TYPE_SPRINT;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_BUTTON_DIVE]) {
        return eWidgetType::TYPE_DIVE;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_BUTTON_SWIM]) {
        return eWidgetType::TYPE_SWIM;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_ACCELERATE]) {
        return eWidgetType::TYPE_ACCELERATE;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_BRAKE]) {
        return eWidgetType::TYPE_BRAKE;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_VEHICLE_SHOOT_LEFT]) {
        return eWidgetType::TYPE_FIRE_LEFT;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_VEHICLE_SHOOT_RIGHT]) {
        return eWidgetType::TYPE_FIRE_RIGHT;
    }

    if (pWidget == widgets[eWidgetIDs::WIDGET_RADAR]) {
        return eWidgetType::TYPE_MAP;
    }

    return eWidgetType::TYPE_NONE;
}

void CWidgets::SetAlpha(int Alpha)
{
    CWidgets::m_iAlpha = std::clamp(Alpha, 0, static_cast<int>(eMaxConfig::MAX_WIDGETALPHA));
}

float CWidgets::GetAlpha()
{
    return static_cast<float>(CWidgets::m_iAlpha);
}

void CWidgets::SetToggle(bool iToggle)
{
	CWidgets::m_bToggleWidget = iToggle;
}

bool CWidgets::IsToggleActive()
{
	return CWidgets::m_bToggleWidget;
}