#include "../../../../Main.h"
#include "CPlayerTags.h"
#include "../Syncs/CRemotePlayer.h"
#include "../../CGame.h"
#include "../../CSprite.h"
#include "../../CCameraAim.h"
#include "../CMultiPlayer.h"
#include "../Guis/CGui.h"
#include "../../Scripts/CScripting.h"

bool CPlayerTags::m_bHealthBarStyle = false;
bool CPlayerTags::m_bWeaponTag = false;

CPlayerTags::CPlayerTags()
{

}

CPlayerTags::~CPlayerTags()
{

}

void CPlayerTags::Render()
{
    if (!CMultiPlayer::GetActiveNetwork() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()) {
        return;
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
        return;
    }

    CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
    if (!pPlayerPool) {
        return;
    }

    //     CVehiclePool* pVehiclePool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool();
    //     if (!pVehiclePool) {
    //         return;
    //     }

    if (CMultiPlayer::GetActiveNetwork()->m_bShowPlayerTags)
    {
        for (const auto& playerPair : pPlayerPool->GetAllRemotePlayers())
        {
            uint16_t playerId = playerPair.first;
            CRemotePlayer* pRemotePlayer = playerPair.second;

            if (!pRemotePlayer || !pRemotePlayer->IsActive() || !pRemotePlayer->IsRemoteNameTag() || pRemotePlayer->GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_WASTED) {
                continue;
            }

            CPlayerPed* pGamePlayer = pRemotePlayer->GetGamePlayer();
            if (!pGamePlayer || !pGamePlayer->VerifyInstance()) {
                continue;
            }

            // CVehicle* pGameVehicle = pRemotePlayer->GetGameVehicle();
            // uint16_t vehicleId = pRemotePlayer->GetRemoteVehicleID();

            // bool isRCVehicle = false;
            // if (pRemotePlayer->GetNetworkPlayerState() == eNetworkPlayerState::PLAYER_STATE_DRIVER && pGameVehicle && pVehiclePool->IsValidPointer(vehicleId, pGameVehicle, false))
            // {
            //     isRCVehicle = pGameVehicle->IsRCVehicle();
            // }

            // if (!isRCVehicle && !pGamePlayer->IsAdded()) {
            //     continue;
            // }

            const float nameTagDrawDistance = CMultiPlayer::GetActiveNetwork()->m_fNameTagDrawDistance;
            const bool inDrawDistance = pGamePlayer->GetDistanceFromCamera() <= nameTagDrawDistance;

            if (inDrawDistance)
            {
                CVector tagPos{};
                // CMatrix matPlayer;
                //
                // if (isRCVehicle)
                // {
                //     pGameVehicle->GetMatrix(&matPlayer);
                //     tagPos.x = matPlayer.pos.x;
                //     tagPos.y = matPlayer.pos.y;
                //     tagPos.z = matPlayer.pos.z;
                // }
                // else
                // {
                if (pGamePlayer->IsAdded())
                {
                    pGamePlayer->GetBonePosition(8, &tagPos);
                }
                else
                {
                    continue;
                }
                // }

                tagPos.z += 0.45f + (pGamePlayer->GetDistanceFromCamera() * 0.0475f);

                CAMERA_AIM *pCam = CCameraAim::GameGetInternalAim();
                int hitEntity = 0;

                if (CMultiPlayer::GetActiveNetwork()->m_bNameTagLOS)
                {
                    hitEntity = CScripting::ScriptCommand(&get_line_of_sight, tagPos.x, tagPos.y, tagPos.z, pCam->pos1x, pCam->pos1y, pCam->pos1z, 1, 0, 0, 1, 0);
                }

                if (!CMultiPlayer::GetActiveNetwork()->m_bNameTagLOS || (hitEntity && !pRemotePlayer->IsRemoteNPC()))
                {
                    CVector screenPos{};
                    if (!RsGlobal || RsGlobal->maximumWidth <= 0 || RsGlobal->maximumHeight <= 0 || !CSprite::CalcScreenCoors(&tagPos, &screenPos, nullptr, nullptr, false, false) || !std::isfinite(screenPos.x) || !std::isfinite(screenPos.y) || !std::isfinite(screenPos.z)) {
                        continue;
                    }

                    screenPos.x *= ImGui::GetIO().DisplaySize.x / static_cast<float>(RsGlobal->maximumWidth);
                    screenPos.y *= ImGui::GetIO().DisplaySize.y / static_cast<float>(RsGlobal->maximumHeight);

                    if (screenPos.z >= 1.0f)
                    {
                        float screen_x = static_cast<float>(screenPos.x);
                        float screen_y = static_cast<float>(screenPos.y);

                        char rawName[64]{};
                        CNecessary::ConvertLocalizedEncodingToUTF8(rawName, (char*)pRemotePlayer->GetRemoteName(), sizeof(rawName));

                        char tempBuff[128]{};
                        snprintf(tempBuff, sizeof(tempBuff), OBFUSCATE("%s (%d)"), rawName, playerId);

                        screen_y -= 11.0f;

                        ImVec2 pos = ImVec2(screen_x, screen_y);
                        uint32_t color = static_cast<uint32_t>(pRemotePlayer->GetRemoteColor());

                        uint8_t r = (color >> 24) & 0xFF;
                        uint8_t g = (color >> 16) & 0xFF;
                        uint8_t b = (color >> 8) & 0xFF;
                        uint8_t a = color & 0xFF;

                        a = 0xFF;

                        uint32_t convertedColor = (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);

                        CGui::GetFontRender()->Render3DLabel(CGui::GetFontRender()->GetFontSize(), pos, (char*)tempBuff, convertedColor);

                        if (!CPlayerTags::m_bHealthBarStyle)
                        {
                            CPlayerTags::DrawComponentsDefault(pRemotePlayer, screenPos, tempBuff);
                        }
                        else
                        {
                            CPlayerTags::DrawComponentsNew(pRemotePlayer, screenPos, tempBuff);
                        }
                    }
                }
            }
        }
    }
}

void CPlayerTags::DrawComponentsDefault(CRemotePlayer* pPlayer, CVector Out, char* tempBuff)
{
    CPlayerTags::HealthBarBDRColor = ImColor( 0x00, 0x00, 0x00, 0xFF );
    CPlayerTags::HealthBarColor = ImColor(0xB9, 0x22, 0x28, 0xFF);
    CPlayerTags::HealthBarBGColor = ImColor(0x4B, 0x0B, 0x14, 0xFF);

    float fWidth = 69.0f;
    float fHeight = 11.5f;
    float fOutline = 2.0f;
    float fSize = CGui::GetFontRender()->GetFontSize();

    bool bAfk = pPlayer->IsRemoteAFK();

    float fHealth = pPlayer->GetReportedHealth();
    if (fHealth <= 0.0f) {
        return;
    }

    float fArmour = pPlayer->GetReportedArmour();

    // top left
    CPlayerTags::HealthBarBDR1.x = Out.x - ((fWidth / 2) + fOutline);
    CPlayerTags::HealthBarBDR1.y = Out.y + ((fSize / 2) * 1.2f);
    // bottom right
    CPlayerTags::HealthBarBDR2.x = Out.x + ((fWidth / 2) + fOutline);
    CPlayerTags::HealthBarBDR2.y = Out.y + ((fSize / 2) * 1.2f) + fHeight;

    // top left
    CPlayerTags::HealthBarBG1.x = CPlayerTags::HealthBarBDR1.x + fOutline;
    CPlayerTags::HealthBarBG1.y = CPlayerTags::HealthBarBDR1.y + fOutline;
    // bottom right
    CPlayerTags::HealthBarBG2.x = CPlayerTags::HealthBarBDR2.x - fOutline;
    CPlayerTags::HealthBarBG2.y = CPlayerTags::HealthBarBDR2.y - fOutline;

    // top left
    CPlayerTags::HealthBar1.x = CPlayerTags::HealthBarBG1.x;
    CPlayerTags::HealthBar1.y = CPlayerTags::HealthBarBG1.y;
    // bottom right
    CPlayerTags::HealthBar2.y = CPlayerTags::HealthBarBG2.y;

    if (fHealth > 100.0f)
    {
        fHealth = 100.0f;
    }

    fHealth *= fWidth / 100.0f;
    fHealth -= (fWidth / 2);
    CPlayerTags::HealthBar2.x = Out.x + fHealth;

    if (fArmour > 0.0f)
    {
        CPlayerTags::HealthBarBDR1.y += 13.0f;
        CPlayerTags::HealthBarBDR2.y += 13.0f;
        CPlayerTags::HealthBarBG1.y += 13.0f;
        CPlayerTags::HealthBarBG2.y += 13.0f;
        CPlayerTags::HealthBar1.y += 13.0f;
        CPlayerTags::HealthBar2.y += 13.0f;
    }

    ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::HealthBarBDR1, CPlayerTags::HealthBarBDR2, CPlayerTags::HealthBarBDRColor);
    ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::HealthBarBG1, CPlayerTags::HealthBarBG2, CPlayerTags::HealthBarBGColor);
    ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::HealthBar1, CPlayerTags::HealthBar2, CPlayerTags::HealthBarColor);

    if (fArmour > 0.0f)
    {
        CPlayerTags::HealthBarBDR1.y -= 13.0f;
        CPlayerTags::HealthBarBDR2.y -= 13.0f;
        CPlayerTags::HealthBarBG1.y -= 13.0f;
        CPlayerTags::HealthBarBG2.y -= 13.0f;
        CPlayerTags::HealthBar1.y -= 13.0f;
        CPlayerTags::HealthBar2.y -= 13.0f;

        CPlayerTags::HealthBarColor = ImColor(200, 200, 200, 255);
        CPlayerTags::HealthBarBGColor = ImColor(40, 40, 40, 255);

        if (fArmour > 100.0f)
        {
            fArmour = 100.0f;
        }

        fArmour *= fWidth / 100.0f;
        fArmour -= (fWidth / 2);
        CPlayerTags::HealthBar2.x = Out.x + fArmour;
        ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::HealthBarBDR1, CPlayerTags::HealthBarBDR2, CPlayerTags::HealthBarBDRColor);
        ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::HealthBarBG1, CPlayerTags::HealthBarBG2, CPlayerTags::HealthBarBGColor);
        ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::HealthBar1, CPlayerTags::HealthBar2, CPlayerTags::HealthBarColor);
    }

    const char* weaponSprite = CPlayerTags::GetWeaponSpriteId(pPlayer->GetGamePlayer()->GetCurrentWeapon());
    ImFont* weaponFont = CGui::GetFontRender()->GetWeaponFont();

    ImGui::PushFont(weaponFont);
    ImVec2 weaponSize = ImGui::CalcTextSize(weaponSprite);
    ImGui::PopFont();

    ImVec2 nameCenter = ImVec2(Out.x, Out.y - 5.0f);
    float nameLeft = nameCenter.x - ImGui::CalcTextSize(tempBuff).x / 2.0f;

    float iconSpacing = 5.0f;
    ImVec2 weaponPos = ImVec2(nameLeft - weaponSize.x - iconSpacing, nameCenter.y);

    if (CPlayerTags::m_bWeaponTag)
    {
        ImGui::PushFont(weaponFont);
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
        ImU32 mainColor = IM_COL32(255, 255, 255, 255);

        drawList->AddText(ImVec2(weaponPos.x - 1, weaponPos.y), outlineColor, weaponSprite);
        drawList->AddText(ImVec2(weaponPos.x + 1, weaponPos.y), outlineColor, weaponSprite);
        drawList->AddText(ImVec2(weaponPos.x, weaponPos.y - 1), outlineColor, weaponSprite);
        drawList->AddText(ImVec2(weaponPos.x, weaponPos.y + 1), outlineColor, weaponSprite);
        drawList->AddText(weaponPos, mainColor, weaponSprite);

        ImGui::PopFont();
    }

    if (bAfk)
    {
        ImVec2 a = ImVec2(CPlayerTags::HealthBarBDR1.x - (fSize * 1.0f), CPlayerTags::HealthBarBDR1.y - 1.0f);

        ImFont* sampFont = CGui::GetFontRender()->GetSampFont();
        ImGui::PushFont(sampFont);

        ImVec2 textSize = ImGui::CalcTextSize(OBFUSCATE("\x45"));
        float paddingX = 5.0f;
        float paddingY = 2.0f;
        ImVec2 bgMin = ImVec2(a.x - paddingX, a.y - paddingY);
        ImVec2 bgMax = ImVec2(a.x + textSize.x + paddingX, a.y + textSize.y + paddingY);

        ImGui::GetBackgroundDrawList()->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);

        ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
        ImU32 mainColor = IM_COL32(255, 255, 255, 255);

        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x - 1, a.y), outlineColor, OBFUSCATE("\x45"));
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x + 1, a.y), outlineColor, OBFUSCATE("\x45"));
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x, a.y - 1), outlineColor, OBFUSCATE("\x45"));
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x, a.y + 1), outlineColor, OBFUSCATE("\x45"));

        ImGui::GetBackgroundDrawList()->AddText(a, mainColor, OBFUSCATE("\x45"));

        ImGui::PopFont();
    }
}

void CPlayerTags::DrawComponentsNew(CRemotePlayer* pPlayer, CVector Out, char* tempBuff)
{


    float fWidth = 100.0f;
    float fHeight = 10.0f;
    float fArmourWidth = 100.0f;
    float fArmourHeight = 5.0f;
    float fOutline = 2.0f;
    float fSize = CGui::GetFontRender()->GetFontSize();

    bool bAfk = pPlayer->IsRemoteAFK();

    float fHealth = pPlayer->GetReportedHealth();
    if (fHealth <= 0.0f) {
        return;
    }

    float fArmour = pPlayer->GetReportedArmour();

    CPlayerTags::HealthBarBG1.x = Out.x - ((fWidth / 2) + fOutline);
    CPlayerTags::HealthBarBG1.y = Out.y + (fSize * 1.2f);
    CPlayerTags::HealthBarBG2.x = Out.x + ((fWidth / 2) + fOutline);
    CPlayerTags::HealthBarBG2.y = Out.y + (fSize * 1.2f) + fHeight;

    ImVec2 shadowOffset(1.0f, 1.0f);
    ImVec2 shadowBG1(CPlayerTags::HealthBarBG1.x - shadowOffset.x, CPlayerTags::HealthBarBG1.y - shadowOffset.y);
    ImVec2 shadowBG2(CPlayerTags::HealthBarBG2.x + shadowOffset.x, CPlayerTags::HealthBarBG2.y + shadowOffset.y);
    ImGui::GetBackgroundDrawList()->AddRect(shadowBG1, shadowBG2, ImColor(0, 0, 0, 64), 1.5f, 0, 1.0f);

    ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::HealthBarBG1, CPlayerTags::HealthBarBG2, ImColor(0.612f, 0.114f, 0.114f, 0.349f), 1.5f);

    CPlayerTags::HealthBar1.x = CPlayerTags::HealthBarBG1.x;
    CPlayerTags::HealthBar1.y = CPlayerTags::HealthBarBG1.y;
    CPlayerTags::HealthBar2.x = Out.x + ((fWidth / 2) + fOutline);
    CPlayerTags::HealthBar2.y = CPlayerTags::HealthBarBG2.y;

    if (fHealth > 100.0f)
    {
        fHealth = 100.0f;
    }

    float totalWidth = fWidth + 2 * fOutline;
    fHealth *= totalWidth / 100.0f;
    fHealth -= (totalWidth / 2);
    CPlayerTags::HealthBar2.x = Out.x + fHealth;

    ImGui::GetBackgroundDrawList()->PushClipRect(ImVec2(CPlayerTags::HealthBar1.x, CPlayerTags::HealthBar1.y - 1.0f), ImVec2(CPlayerTags::HealthBar2.x, CPlayerTags::HealthBar2.y + 1.0f), true);

    ImVec2 shadowHealth1(CPlayerTags::HealthBar1.x - shadowOffset.x, CPlayerTags::HealthBar1.y - shadowOffset.y);
    ImVec2 shadowHealth2(CPlayerTags::HealthBar2.x + shadowOffset.x, CPlayerTags::HealthBar2.y + shadowOffset.y);
    ImGui::GetBackgroundDrawList()->AddRect(shadowHealth1, shadowHealth2, ImColor(0, 0, 0, 64), 1.5f, 0, 1.0f);

    float rounding = 1.5f;
    float step = 0.5f;
    int steps = static_cast<int>(rounding / step);

    for (int i = 0; i <= steps; i++)
    {
        float offset = i * step;
        float alpha = 1.0f - (offset / rounding);
        ImVec2 topLeft = ImVec2(CPlayerTags::HealthBarBG1.x + offset, CPlayerTags::HealthBarBG1.y + offset);
        ImVec2 bottomRight = ImVec2(CPlayerTags::HealthBarBG2.x - offset, CPlayerTags::HealthBarBG2.y - offset);

        ImGui::GetBackgroundDrawList()->AddRectFilledMultiColor(topLeft, bottomRight, ImColor(0, 0, 0, static_cast<int>(255 * alpha)), ImColor(255, 0, 0, static_cast<int>(255 * alpha)), ImColor(255, 0, 0, static_cast<int>(255 * alpha)), ImColor(0, 0, 0, static_cast<int>(255 * alpha)));
    }

    ImGui::GetBackgroundDrawList()->PopClipRect();

    if (fArmour > 0.0f)
    {
        CPlayerTags::ArmourBarBG1.x = Out.x - ((fArmourWidth / 2) + fOutline);
        CPlayerTags::ArmourBarBG1.y = Out.y + (fSize * 1.2f);
        CPlayerTags::ArmourBarBG2.x = Out.x + ((fArmourWidth / 2) + fOutline);
        CPlayerTags::ArmourBarBG2.y = Out.y + (fSize * 1.2f) + fArmourHeight;

        CPlayerTags::ArmourBar1.x = CPlayerTags::ArmourBarBG1.x;
        CPlayerTags::ArmourBar1.y = CPlayerTags::HealthBarBG1.y;
        CPlayerTags::ArmourBar2.y = CPlayerTags::ArmourBarBG2.y;

        CPlayerTags::ArmourBarBG1.y += 13.0f;
        CPlayerTags::ArmourBarBG2.y += 13.0f;
        CPlayerTags::ArmourBar1.y += 13.0f;
        CPlayerTags::ArmourBar2.y += 13.0f;

        if (fArmour > 100.0f)
        {
            fArmour = 100.0f;
        }

        fArmour *= fArmourWidth / 100.0f;
        fArmour -= (fArmourWidth / 2);
        CPlayerTags::ArmourBar2.x = Out.x + fArmour;

        ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::ArmourBarBG1, CPlayerTags::ArmourBarBG2, ImColor(0.06f, 0.05f, 0.07f, 0.70f));
        ImGui::GetBackgroundDrawList()->AddRectFilled(CPlayerTags::ArmourBar1, CPlayerTags::ArmourBar2, ImColor(200, 200, 200, 255));
    }

    // ระบบ Icon Chat ตอนกำลังเปิดคีย์บอร์ดพิมพ์ (เดียวมาเชือมที่หลัง)
    // ImVec2 textSize = ImGui::CalcTextSize(tempBuff);
    // float typingBoxWidth = 40.0f;
    // float typingBoxHeight = 30.0f;
    // float dotSpacing = 12.0f;
    // float baseDotSize = 3.0f;
    // float maxDotSize = 5.0f;

    // ImVec2 typingBoxPos = ImVec2(Out.x - (typingBoxWidth / 2), Out.y - typingBoxHeight - 5.0f);

    // ImGui::GetBackgroundDrawList()->AddRectFilled(typingBoxPos, ImVec2(typingBoxPos.x + typingBoxWidth, typingBoxPos.y + typingBoxHeight), ImColor(0.2f, 0.2f, 0.2f, 0.8f), 5.0f);

    // static float animTime = 0.0f;
    // animTime = ImGui::GetTime();
    // float animSpeed = 1.0f;
    // int activeDot = static_cast<int>(animTime / animSpeed) % 3;
    // float animPhase = fmod(animTime, animSpeed) / animSpeed;

    // for (int i = 0; i < 3; i++)
    // {
    //     float dotSize = baseDotSize;
    //     if (i == activeDot)
    //     {
    //         dotSize = baseDotSize + (maxDotSize - baseDotSize) * (1.0f - animPhase);
    //     }
    //     else if (i == (activeDot + 1) % 3)
    //     {
    //         dotSize = baseDotSize + (maxDotSize - baseDotSize) * animPhase;
    //     }

    //     ImVec2 dotPos = ImVec2(typingBoxPos.x + (typingBoxWidth / 2) - (dotSpacing) + (i * dotSpacing), typingBoxPos.y + (typingBoxHeight / 2));

    //     ImGui::GetBackgroundDrawList()->AddCircleFilled(dotPos, dotSize, ImColor(1.0f, 1.0f, 1.0f, 1.0f));
    // }

    const char* weaponSprite = CPlayerTags::GetWeaponSpriteId(pPlayer->GetGamePlayer()->GetCurrentWeapon());
    ImFont* weaponFont = CGui::GetFontRender()->GetWeaponFont();

    ImGui::PushFont(weaponFont);
    ImVec2 weaponSize = ImGui::CalcTextSize(weaponSprite);
    ImGui::PopFont();

    ImVec2 nameCenter = ImVec2(Out.x, Out.y - 5.0f);
    float nameLeft = nameCenter.x - ImGui::CalcTextSize(tempBuff).x / 2.0f;

    float afkSize = fSize * 1.3f;
    ImVec2 afkPos = ImVec2(nameLeft - afkSize - 1.0f, nameCenter.y);

    if (CPlayerTags::m_bWeaponTag)
    {
        ImVec2 weaponPos;

        if (bAfk)
        {
            weaponPos = ImVec2(afkPos.x - weaponSize.x - 1.0f, nameCenter.y);
        }
        else
        {
            weaponPos = afkPos;
        }

        ImGui::PushFont(weaponFont);
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
        ImU32 mainColor = IM_COL32(255, 255, 255, 255);

        drawList->AddText(ImVec2(weaponPos.x - 1, weaponPos.y), outlineColor, weaponSprite);
        drawList->AddText(ImVec2(weaponPos.x + 1, weaponPos.y), outlineColor, weaponSprite);
        drawList->AddText(ImVec2(weaponPos.x, weaponPos.y - 1), outlineColor, weaponSprite);
        drawList->AddText(ImVec2(weaponPos.x, weaponPos.y + 1), outlineColor, weaponSprite);

        drawList->AddText(weaponPos, mainColor, weaponSprite);
        ImGui::PopFont();
    }

    if (bAfk)
    {
        ImVec2 a = afkPos;
        ImFont* sampFont = CGui::GetFontRender()->GetSampFont();
        ImGui::PushFont(sampFont);

        ImVec2 textSize = ImGui::CalcTextSize(OBFUSCATE("\x45"));
        float paddingX = 5.0f;
        float paddingY = 2.0f;
        ImVec2 bgMin = ImVec2(a.x - paddingX, a.y - paddingY);
        ImVec2 bgMax = ImVec2(a.x + textSize.x + paddingX, a.y + textSize.y + paddingY);

        ImGui::GetBackgroundDrawList()->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 3.0f);

        ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
        ImU32 mainColor = IM_COL32(255, 255, 255, 255);

        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x - 1, a.y), outlineColor, OBFUSCATE("\x45"));
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x + 1, a.y), outlineColor, OBFUSCATE("\x45"));
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x, a.y - 1), outlineColor, OBFUSCATE("\x45"));
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(a.x, a.y + 1), outlineColor, OBFUSCATE("\x45"));

        ImGui::GetBackgroundDrawList()->AddText(a, mainColor, OBFUSCATE("\x45"));

        ImGui::PopFont();
    }
}

const char* CPlayerTags::GetWeaponSpriteId(uint8_t nWeapon)
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
            return OBFUSCATE("%");
        }
    }
}

void CPlayerTags::SetStyleHealthBar(bool bEnabled)
{
    CPlayerTags::m_bHealthBarStyle = bEnabled;
}

void CPlayerTags::SetWeaponTag(bool bEnable)
{
    CPlayerTags::m_bWeaponTag = bEnable;
}