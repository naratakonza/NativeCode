#include "../../Main.h"
#include "CHud.h"
#include "CAspectRatio.h"
#include "CCamera.h"
#include "CDraw.h"
#include "Enums/eMaxConfig.h"
#include "Enums/eWeaponSampid.h"
#include "Multiplayer/CMultiPlayer.h"

#include <mach-o/dyld.h>

int16_t& CHud::m_ItemToFlash = *reinterpret_cast<int16_t*>(SA_ADDR(0x7ACF00) + _dyld_get_image_vmaddr_slide(0));
int CHud::m_iAlpha = 255;

static constexpr float ar43 = 4.0f / 3.0f;
static bool drawCrosshairsHookInstalled = false;

extern "C" __attribute__((visibility("default"))) void CHud_DrawCrosshairs_hook(void (*CHud_DrawCrosshairs)())
{
    if (!drawCrosshairsHookInstalled || !CMultiPlayer::IsSampActive())
    {
        CHud_DrawCrosshairs();
        return;
    }

    const float aspectRatio = CDraw::ms_fAspectRatio;
    if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0f)
    {
        CHud_DrawCrosshairs();
        return;
    }

    float sliderFOV = CDraw::GetFov();
    if (sliderFOV <= 0.0f)
    {
        sliderFOV = 77.0f;
    }

    const float fovScale = tanf(77.0f * 3.14159265358979323846f / 360.0f) / tanf(sliderFOV * 3.14159265358979323846f / 360.0f);
    const float fovOffsetX = (0.530f - 0.5f) * (fovScale - 1.0f);
    const float fovOffsetY = (0.400f - 0.5f) * (fovScale - 1.0f);
    const float fAspectCorrection = aspectRatio - ar43;

    const float save1 = CCamera::m_f3rdPersonCHairMultX;
    const float save2 = CCamera::m_f3rdPersonCHairMultY;
    const auto* network = CMultiPlayer::GetActiveNetwork();
    CPlayerPed* pPlayer = network ? CGame::GetPlayer() : nullptr;
    const bool rifle = pPlayer && pPlayer->GetCurrentWeapon() == eWeaponSampId::WEAPON_RIFLE;

    if (CAspectRatio::ms_fAspectRatioSize > 0.0f)
    {
        const float yRatioScale = CAspectRatio::ms_fAspectRatioSize / aspectRatio;
        if (rifle)
        {
            const float aspectX = 0.530f - 0.84f * ar43 * 0.01115f;
            const float oldAspectY = 0.400f + 0.84f * ar43 * 0.038f;
            const float newYOffset = (oldAspectY - 0.5f) * yRatioScale * fovScale;
            CCamera::m_f3rdPersonCHairMultX = 0.5f + (aspectX - 0.5f) * fovScale;
            CCamera::m_f3rdPersonCHairMultY = 0.5f + newYOffset;
        }
        else
        {
            const float aspectX = 0.530f - fAspectCorrection * 0.01115f;
            const float oldAspectY = 0.400f + fAspectCorrection * 0.038f;
            const float newYOffset = (oldAspectY - 0.5f) * yRatioScale * fovScale;
            CCamera::m_f3rdPersonCHairMultX = 0.5f + (aspectX - 0.5f) * fovScale;
            CCamera::m_f3rdPersonCHairMultY = 0.5f + newYOffset;
        }
    }
    else
    {
        if (rifle)
        {
            const float aspectX = 0.530f - 0.84f * ar43 * 0.01115f;
            const float aspectY = 0.400f + 0.84f * ar43 * 0.038f;
            CCamera::m_f3rdPersonCHairMultX = aspectX + fovOffsetX;
            CCamera::m_f3rdPersonCHairMultY = aspectY + fovOffsetY;
        }
        else
        {
            const float aspectX = 0.530f - fAspectCorrection * 0.01115f;
            const float aspectY = 0.400f + fAspectCorrection * 0.038f;
            CCamera::m_f3rdPersonCHairMultX = aspectX + fovOffsetX;
            CCamera::m_f3rdPersonCHairMultY = aspectY + fovOffsetY;
        }
    }

    CHud_DrawCrosshairs();

    CCamera::m_f3rdPersonCHairMultX = save1;
    CCamera::m_f3rdPersonCHairMultY = save2;
}

void CHud::Initialise()
{
    CNecessary::Log(OBFUSCATE("CHud Initialise"));

    drawCrosshairsHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x218378), "\xA1\xFC\x09\x94") && CMemory::HasMappedBytes(SA_ADDR(0x2154E0), "\xC8\x28\x00\x90\x08\x01\x33\x91\x00\x01\x40\xBD", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x2154F4), "\xC8\x28\x00\x90\x08\x11\x33\x91\x00\x91\x40\x0D", 12, "__text");
    if (!drawCrosshairsHookInstalled)
    {
        CNecessary::Log(OBFUSCATE("CHud DrawCrosshairs bridge unavailable"));
    }
}

void CHud::SetAlpha(int Alpha)
{
    CHud::m_iAlpha = std::clamp(Alpha, 0, static_cast<int>(eMaxConfig::MAX_WIDGETALPHA));
}

float CHud::GetAlpha()
{
    return static_cast<float>(CHud::m_iAlpha);
}