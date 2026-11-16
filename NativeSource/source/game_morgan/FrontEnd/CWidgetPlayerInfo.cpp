#include "../../../Main.h"
#include "CWidgetPlayerInfo.h"
#include "../CHud.h"
#include "../CSprite2d.h"
#include "../CTimer.h"
#include "../CWorld.h"
#include "../Entity/Ped/CPedGta.h"
#include "../Multiplayer/CMultiPlayer.h"

#include <mach-o/dyld.h>

static bool renderArmorBarHookInstalled = false;

void CWidgetPlayerInfo::Initialise()
{
    CNecessary::Log(OBFUSCATE("CWidgetPlayerInfo Initialise"));

    renderArmorBarHookInstalled = CMemory::HasMappedBytes(SA_ADDR(0x20931C), "\xAD\x38\x0A\x14\xED\x33\x01\x6D", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x4975D0), "\x82\x00\x00\x10\xF0\x00\x00\xD0\x10\x72\x42\xF9\x00\x02\x1F\xD6\xFF\x43\x01\xD1\x4F\xC7\xF5\x17", 24, "__mg_gameproc") && CMemory::HasMappedBytes(SA_ADDR(0x209338), "\x08\x2D\x00\xF0\x08\x01\x3C\x91\x08\x01\x40\x79", 12, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x209408), "\x20\x2D\x00\xB0\x00\x80\x0D\x91", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x303374), "\xFF\x43\x03\xD1\xEF\x3B\x04\x6D\xED\x33\x05\x6D", 12, "__text");
    if (!renderArmorBarHookInstalled)
    {
        CNecessary::Log(OBFUSCATE("CWidgetPlayerInfo RenderArmorBar bridge unavailable"));
    }
}

extern "C" __attribute__((visibility("default"))) void CWidgetPlayerInfo_RenderArmorBar_hook(uintptr_t thiz, int Player, float fTop, void (*original)(uintptr_t, int, float))
{
    if (renderArmorBarHookInstalled)
    {
        CWidgetPlayerInfo::RenderArmorBar(thiz, Player, fTop);
        return;
    }

    original(thiz, Player, fTop);
}

extern "C" __attribute__((visibility("default"))) void CWidgetPlayerInfo_Draw_hook(uintptr_t thiz, void (*Draw)(uintptr_t))
{
    if (!CMultiPlayer::IsSampActive())
    {
        Draw(thiz);
        return;
    }

    const auto maxAlpha = static_cast<std::uint8_t>(CHud::GetAlpha());
    auto& alpha = *reinterpret_cast<std::uint8_t*>(thiz + 0x58);
    const auto previousAlpha = alpha;
    if (previousAlpha <= maxAlpha)
    {
        Draw(thiz);
        return;
    }

    // Keep the engine's fade state and limit alpha only while drawing Player Info.
    alpha = maxAlpha;
    Draw(thiz);
    alpha = previousAlpha;
}

void CWidgetPlayerInfo::RenderArmorBar(uintptr_t thiz, int Player, float fTop)
{
    if (!renderArmorBarHookInstalled || !thiz || !CWorld::Players) {
        return;
    }

    const auto* ped = CWorld::GetPlayerPed(static_cast<unsigned int>(Player));
    if (!ped) {
        return;
    }

    const float m_fArmour = ped->fArmour;
    if (CHud::m_ItemToFlash == 3)
    {
        if (CTimer::m_snTimeInMilliseconds % 0x15E < 0x97) {
            return;
        }
    }
    else
    {
        if (m_fArmour <= 1.0f) {
            return;
        }
    }

    const float left = *reinterpret_cast<const float*>(thiz + 0x2C); // m_RectScreen.left
    const float right = *reinterpret_cast<const float*>(thiz + 0x34); // m_RectScreen.right
    const float bottom = *reinterpret_cast<const float*>(thiz + 0x38); // m_RectScreen.bottom
    const float top = *reinterpret_cast<const float*>(thiz + 0x30); // m_RectScreen.top

    const float width = std::fabs(right - left);
    const float heightDiff = std::fabs(top - bottom);
    if (!std::isfinite(width) || !std::isfinite(heightDiff) || !std::isfinite(fTop) || !std::isfinite(m_fArmour)) {
        return;
    }
    
    const auto barHeight = static_cast<uint8_t>(std::min(heightDiff * 0.095f, static_cast<float>(UINT8_MAX)));

    const uint8_t maxArmorByte = reinterpret_cast<const uint8_t*>(&CWorld::Players[Player])[0x18C];
    float maxArmor = static_cast<float>(maxArmorByte);
    if (maxArmor <= 0.0f)
    {
        maxArmor = 100.0f;
    }

    const float armorRatio = std::fmin(maxArmor / 176.0f, 1.0f);
    auto barWidth = static_cast<uint16_t>(std::min(width * armorRatio * 0.55f, static_cast<float>(UINT16_MAX)));
    if (barWidth == 0)
    {
        barWidth = 176;
    }

    const float barX = std::trunc(left + (width * 0.55f) - barWidth);
    const float percent = std::fmin(m_fArmour / maxArmor, 1.0f) * 100.0f;
    const uint8_t alpha = *reinterpret_cast<const uint8_t*>(thiz + 0x58); // m_Color.alpha
    static const auto* gpCHudColours = reinterpret_cast<const uint8_t*>(SA_ADDR(0x7AE360) + _dyld_get_image_vmaddr_slide(0));
    const CRGBA hudColor(gpCHudColours[5 * 4], gpCHudColours[5 * 4 + 1], gpCHudColours[5 * 4 + 2], alpha);
    CRGBA backgroundColor(0, 0, 0, 0);

    CSprite2d::DrawBarChart(barX, fTop, barWidth, barHeight, percent, 0, 0, 1, hudColor, reinterpret_cast<uint8_t*>(&backgroundColor));
}
