#include "../../../../Main.h"
#include "CDeathWindow.h"
#include "../Guis/CGui.h"
// #include "../Guis/CVoiceMenu.h"
#include "../CMultiPlayer.h"
#include "../../Enums/eWeaponSampid.h"

CDeathWindow::CDeathWindow()
{
    CDeathWindow::m_pDeathWindow.clear();
}

CDeathWindow::~CDeathWindow()
{
    CDeathWindow::m_pDeathWindow.clear();
}

void CDeathWindow::AddEntry(const std::string& victimName, uint32_t victimColor, const std::string& attackerName, uint32_t attackerColor, uint8_t deathReason, uint32_t fontcolor)
{
    if (victimName.empty()) {
        return;
    }

    auto deathEntry = std::make_shared<DeathWindowStruct>();
    deathEntry->victimName = victimName;
    deathEntry->victimColor = victimColor;
    deathEntry->attackerName = attackerName;
    deathEntry->attackerColor = attackerColor;
    deathEntry->deathReason = deathReason;
    deathEntry->fontcolor = fontcolor;

    if (CDeathWindow::m_pDeathWindow.size() >= 5)
    {
        CDeathWindow::m_pDeathWindow.pop_front();
    }

    CDeathWindow::m_pDeathWindow.push_back(deathEntry);
}

void CDeathWindow::Render()
{
    if (CDeathWindow::m_pDeathWindow.empty()) {
        return;
    }

    if (!CMultiPlayer::GetActiveNetwork() || CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
        return;
    }

    if (!CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()) {
        return;
    }

    // if (CGui::GetScoreBoard()->GetActiveScore() || CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() || (CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CVoiceMenu::m_bCustomizeMode) /*|| CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/) {
    //     return;
    // }

    ImGuiIO &io = ImGui::GetIO();
    float deathWindowX = io.DisplaySize.x / 2.0f + SCALE_X(io, 580.0f);

    ImVec2 textPos;
    textPos.y = io.DisplaySize.y / 2.0f - SCALE_Y(io, 170.0f);

    for (const auto& deathInfo : CDeathWindow::m_pDeathWindow)
    {
        if (!deathInfo) {
            continue;
        }

        char victimName[24] = {0}, attackerName[24] = {0};

        CNecessary::ConvertLocalizedEncodingToUTF8(victimName, deathInfo->victimName.c_str(), sizeof(victimName));
        CNecessary::ConvertLocalizedEncodingToUTF8(attackerName, deathInfo->attackerName.c_str(), sizeof(attackerName));

        ImVec2 attackerTextSize = ImGui::CalcTextSize(attackerName);
        float textSpacingX = ImGui::GetStyle().ItemSpacing.x;

        if (deathInfo->deathReason == 255)
        {
            if (!CNecessary::IsValidName(attackerName) && !CNecessary::IsValidName(victimName))
            {
                textPos.x = deathWindowX - attackerTextSize.x - textSpacingX;
                CGui::GetFontRender()->RenderTextDeathMessage(textPos, 0xFFFFFFFF, true, OBFUSCATE("  "));

                textPos.x = deathWindowX;
                CGui::GetFontRender()->RenderTextDeathMessage(textPos, 0xFFFFFFFF, false, OBFUSCATE("  "), nullptr, 0.0f, CGui::GetFontRender()->GetWeaponFont());

                textPos.x += CGui::GetFontRender()->GetFontSize() + 2 * textSpacingX;
                CGui::GetFontRender()->RenderTextDeathMessage(textPos, 0xFFFFFFFF, true, OBFUSCATE("  "));
            }
            else
            {
                textPos.x = deathWindowX - attackerTextSize.x - textSpacingX;
                CGui::GetFontRender()->RenderTextDeathMessage(textPos, deathInfo->fontcolor, true, attackerName);

                textPos.x = deathWindowX;
                CGui::GetFontRender()->RenderTextDeathMessage(textPos, 0xFF000000, false, OBFUSCATE("G"), nullptr, 0.0f, CGui::GetFontRender()->GetWeaponFont());
                CGui::GetFontRender()->RenderTextDeathMessage(textPos, deathInfo->fontcolor, false, CDeathWindow::GetWeaponSpriteId(deathInfo->deathReason), nullptr, 0.0f, CGui::GetFontRender()->GetWeaponFont());

                textPos.x += CGui::GetFontRender()->GetFontSize() + 2 * textSpacingX;
                CGui::GetFontRender()->RenderTextDeathMessage(textPos, deathInfo->fontcolor, true, victimName);
            }
        }
        else
        {
            uint8_t victimColor[3] = {
                static_cast<uint8_t>((deathInfo->victimColor - 0xFF000000) >> 16),
                static_cast<uint8_t>(((deathInfo->victimColor - 0xFF000000) & 0x00ff00) >> 8),
                static_cast<uint8_t>((deathInfo->victimColor - 0xFF000000) & 0x0000ff)
            };

            uint8_t attackerColor[3] = {
                static_cast<uint8_t>((deathInfo->attackerColor - 0xFF000000) >> 16),
                static_cast<uint8_t>(((deathInfo->attackerColor - 0xFF000000) & 0x00ff00) >> 8),
                static_cast<uint8_t>((deathInfo->attackerColor - 0xFF000000) & 0x0000ff)
            };

            textPos.x = deathWindowX - attackerTextSize.x - textSpacingX;
            CGui::GetFontRender()->RenderTextDeathMessage(textPos, ImColor(attackerColor[0], attackerColor[1], attackerColor[2]), true, attackerName);

            textPos.x = deathWindowX;
            CGui::GetFontRender()->RenderTextDeathMessage(textPos, 0xFF000000, false, OBFUSCATE("G"), nullptr, 0.0f, CGui::GetFontRender()->GetWeaponFont());

            CGui::GetFontRender()->RenderTextDeathMessage(textPos, deathInfo->fontcolor, false, CDeathWindow::GetWeaponSpriteId(deathInfo->deathReason), nullptr, 0.0f, CGui::GetFontRender()->GetWeaponFont());

            textPos.x += CGui::GetFontRender()->GetFontSize() + 2 * textSpacingX;
            CGui::GetFontRender()->RenderTextDeathMessage(textPos, ImColor(victimColor[0], victimColor[1], victimColor[2]), true, victimName);
        }

        textPos.y += CGui::GetFontRender()->GetFontSize() + ImGui::GetStyle().ItemSpacing.y;
    }
}

const char* CDeathWindow::GetWeaponSpriteId(uint8_t nWeapon)
{
    switch (nWeapon) {
        case eWeaponSampId::WEAPON_FIST: {
            return OBFUSCATE("%");
        }

        case eWeaponSampId::WEAPON_BRASSKNUCKLE: {
            return OBFUSCATE("B");
        }

        case eWeaponSampId::WEAPON_GOLFCLUB: {
            return OBFUSCATE(">");
        }

        case eWeaponSampId::WEAPON_NITESTICK: {
            return OBFUSCATE("(");
        }

        case eWeaponSampId::WEAPON_KNIFE: {
            return OBFUSCATE("C");
        }

        case eWeaponSampId::WEAPON_BAT: {
            return OBFUSCATE("?");
        }

        case eWeaponSampId::WEAPON_SHOVEL: {
            return OBFUSCATE("&");
        }

        case eWeaponSampId::WEAPON_POOLSTICK: {
            return "\"";
        }

        case eWeaponSampId::WEAPON_KATANA: {
            return OBFUSCATE("!");
        }

        case eWeaponSampId::WEAPON_CHAINSAW:{
            return OBFUSCATE("1");
        }

        case eWeaponSampId::WEAPON_DILDO: {

        }

        case eWeaponSampId::WEAPON_DILDO2: {

        }

        case eWeaponSampId::WEAPON_VIBRATOR: {

        }

        case eWeaponSampId::WEAPON_VIBRATOR2: {
            return OBFUSCATE("E");
        }

        case eWeaponSampId::WEAPON_FLOWER: {
            return OBFUSCATE("$");
        }

        case eWeaponSampId::WEAPON_CANE: {
            return OBFUSCATE("#");
        }

        case eWeaponSampId::WEAPON_GRENADE: {
            return OBFUSCATE("@");
        }

        case eWeaponSampId::WEAPON_TEARGAS: {
            return OBFUSCATE("D");
        }

        case eWeaponSampId::WEAPON_COLT45: {
            return OBFUSCATE("6");
        }

        case eWeaponSampId::WEAPON_SILENCED: {
            return OBFUSCATE("2");
        }

        case eWeaponSampId::WEAPON_DEAGLE: {
            return OBFUSCATE("3");
        }

        case eWeaponSampId::WEAPON_SHOTGUN: {
            return OBFUSCATE("=");
        }

        case eWeaponSampId::WEAPON_SAWEDOFF: {
            return OBFUSCATE("0");
        }

        case eWeaponSampId::WEAPON_SHOTGSPA: {
            return OBFUSCATE("+");
        }

        case eWeaponSampId::WEAPON_UZI: {
            return OBFUSCATE("I");
        }

        case eWeaponSampId::WEAPON_MP5: {
            return OBFUSCATE("8");
        }

        case eWeaponSampId::WEAPON_AK47: {
            return OBFUSCATE("H");
        }

        case eWeaponSampId::WEAPON_M4: {
            return OBFUSCATE("5");
        }

        case eWeaponSampId::WEAPON_TEC9: {
            return OBFUSCATE("7");
        }

        case eWeaponSampId::WEAPON_RIFLE: {
            return OBFUSCATE(".");
        }

        case eWeaponSampId::WEAPON_SNIPER: {
            return OBFUSCATE("A");
        }

        case eWeaponSampId::WEAPON_ROCKETLAUNCHER: {
            return OBFUSCATE("4");
        }

        case eWeaponSampId::WEAPON_HEATSEEKER: {
            return OBFUSCATE(")");
        }

        case eWeaponSampId::WEAPON_FLAMETHROWER: {
            return OBFUSCATE("P");
        }

        case eWeaponSampId::WEAPON_MINIGUN: {
            return OBFUSCATE("F");
        }

        case eWeaponSampId::WEAPON_SATCHEL: {
            return OBFUSCATE("<");
        }

        case eWeaponSampId::WEAPON_BOMB: {
            return OBFUSCATE(";");
        }

        case eWeaponSampId::WEAPON_SPRAYCAN: {
            return OBFUSCATE("/");
        }

        case eWeaponSampId::WEAPON_FIREEXTINGUISHER: {
            return OBFUSCATE(",");
        }

        case eWeaponSampId::WEAPON_PARACHUTE: {
            return OBFUSCATE(":");
        }

        case eWeaponSampId::WEAPON_VEHICLE: {
            return OBFUSCATE("L");
        }

        case eWeaponSampId::WEAPON_DROWN: {
            return OBFUSCATE("J");
        }

        case eWeaponSampId::WEAPON_HELIBLADES: {
            return OBFUSCATE("R");
        }

        case eWeaponSampId::WEAPON_EXPLOSION: {
            return OBFUSCATE("Q");
        }

        case eWeaponSampId::WEAPON_COLLISION: {
            return OBFUSCATE("K");
        }

        case eWeaponSampId::WEAPON_SPECIAL_CONNECT: {

        }

        case eWeaponSampId::WEAPON_SPECIAL_DISCONNECT: {
            return OBFUSCATE("N");
        }

        default: {
            return OBFUSCATE("J");
        }
    }
}