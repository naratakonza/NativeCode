#include "../../../../Main.h"
#include "CChatBubble.h"
#include "../../CGame.h"
#include "../../CSprite.h"
#include "../CMultiPlayer.h"
#include "../Guis/CGui.h"

void CChatBubble::Render()
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

    for (const auto& playerPair : pPlayerPool->GetAllRemotePlayers())
    {
        CRemotePlayer* pRemotePlayer = playerPair.second;

        if (pRemotePlayer && pRemotePlayer->IsActive() && pRemotePlayer->GetNetworkPlayerState())
        {
            if (!pRemotePlayer->m_bChatBubbleIsActive || pRemotePlayer->m_szChatBubbleMessage[0] == '\0') {
                continue;
            }

            CPlayerPed* pGamePlayer = pRemotePlayer->GetGamePlayer();
            if (!pGamePlayer || !pGamePlayer->VerifyInstance()) {
                continue;
            }

            if (!pGamePlayer->IsAdded()) {
                continue;
            }

            const float cbDrawDistance = pRemotePlayer->m_fChatBubbleDrawDistance;
            const bool inDrawDistance = pGamePlayer->GetDistanceFromLocalPlayerPed() <= cbDrawDistance;

            if (inDrawDistance)
            {
                CVector tagPos;
                pGamePlayer->GetBonePosition(8, &tagPos);
                tagPos.z += 0.45f + (pGamePlayer->GetDistanceFromCamera() * 0.0675f);

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

                    screen_y -= 20.0f;

                    ImVec2 pos = ImVec2(screen_x, screen_y);
                    uint32_t color = static_cast<uint32_t>(pRemotePlayer->m_dwChatBubbleColor);

                    CGui::GetFontRender()->Render3DLabel(CGui::GetFontRender()->GetFontSize(), ImVec2(pos.x, pos.y), static_cast<char*>(pRemotePlayer->m_szChatBubbleMessage), color);
                }
            }
        }
    }
}