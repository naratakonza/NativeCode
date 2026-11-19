#include "../../../../Main.h"
#include "../Guis/CGui.h"
#include "CTextLabel.h"
#include "../../CGame.h"
#include "../../CUtil.h"
#include "../../CSprite.h"
#include "../../CCommon.h"
#include "../../CCameraAim.h"
#include "../CMultiPlayer.h"
#include "../../Scripts/CScripting.h"

CTextLabel::CTextLabel(uint16_t id, char* text, uint32_t color, CVector vecPos, float drawDistance, uint8_t useLOS, uint16_t attachedToPlayerID, uint16_t attachedToVehicleID)
{
    CNecessary::ConvertLocalizedEncodingToUTF8(CTextLabel::m_szText, text, sizeof(CTextLabel::m_szText));
    CNecessary::ConvertLocalizedEncodingToUTF8(CTextLabel::m_szTextWithoutColors, text, sizeof(CTextLabel::m_szTextWithoutColors));
    CUtil::FilterColors(CTextLabel::m_szTextWithoutColors);

    CTextLabel::m_id = id;
    CTextLabel::m_dwColor = color;
    CTextLabel::m_vecPos = vecPos;
    CTextLabel::m_fDrawDistance = drawDistance;
    CTextLabel::m_iVirtualWorld = 0;
    CTextLabel::m_bUseLineOfSight = useLOS;
    CTextLabel::m_attachedToPlayerID = attachedToPlayerID;
    CTextLabel::m_attachedToVehicleID = attachedToVehicleID;
    CTextLabel::m_vecOffsetCoords = vecPos;
}

CTextLabel::~CTextLabel()
{

}

void CTextLabel::Render()
{
    if (CTextLabel::m_attachedToPlayerID != eInvalidId::INVALID_PLAYER_ID)
    {
        return CTextLabel::RenderAttachedPlayer();
    }
    else if (CTextLabel::m_attachedToVehicleID != eInvalidId::INVALID_VEHICLE_ID)
    {
        // return CTextLabel::RenderAttachedVehicle(); // Vehicle
        return;
    }
    else
    {
        return CTextLabel::RenderPos();
    }
}

bool CTextLabel::IsTargetDistance(CVector f)
{
    if (!CGame::GetPlayer()) {
        return false;
    }

    return CGame::GetPlayer()->GetDistanceFromPoint(f.x, f.y, f.z) <= CTextLabel::m_fDrawDistance;
}

void CTextLabel::RenderAttachedPlayer()
{
    if (!CMultiPlayer::GetActiveNetwork()) {
        return;
    }

    CPlayerPool* pPlayerPool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool();
    if (!pPlayerPool) {
        return;
    }

    if (CTextLabel::m_attachedToPlayerID == pPlayerPool->GetLocalPlayer()->GetLocalID()) {
        return;
    }

    CRemotePlayer* pRemotePlayer = pPlayerPool->GetRemotePlayer(CTextLabel::m_attachedToPlayerID);
    if (!pRemotePlayer) {
        return;
    }

    CPlayerPed* pPlayer = pRemotePlayer->GetGamePlayer();
    if (!pPlayer) {
        return;
    }

    if (!pPlayer->VerifyInstance()) {
        return;
    }

    if (!pPlayer->IsAdded() || pPlayer->GetDistanceFromLocalPlayerPed() > CTextLabel::m_fDrawDistance) {
        return;
    }

    CVector textPos;
    CVector matPlayer;

    pPlayer->GetBonePosition(8, &matPlayer);

    textPos.x = matPlayer.x + CTextLabel::m_vecOffsetCoords.x;
    textPos.y = matPlayer.y + CTextLabel::m_vecOffsetCoords.y;
    textPos.z = (pPlayer->GetDistanceFromCamera() * 0.055f) + CTextLabel::m_vecOffsetCoords.z + matPlayer.z;

    CVector Out{};
    if (!RsGlobal || RsGlobal->maximumWidth <= 0 || RsGlobal->maximumHeight <= 0 || !CSprite::CalcScreenCoors(&textPos, &Out, nullptr, nullptr, false, false) || !std::isfinite(Out.x) || !std::isfinite(Out.y) || !std::isfinite(Out.z)) {
        return;
    }

    Out.x *= ImGui::GetIO().DisplaySize.x / static_cast<float>(RsGlobal->maximumWidth);
    Out.y *= ImGui::GetIO().DisplaySize.y / static_cast<float>(RsGlobal->maximumHeight);

    if (Out.z < 1.0f) {
        return;
    }

    int hitEntity = 0;
    if (CTextLabel::IsLosSceneUsing(&hitEntity, &textPos))
    {
        if (!hitEntity)
        {
            return;
        }
    }

    ImVec2 pos(static_cast<int>(Out.x), static_cast<int>(Out.y));

    std::string szText(CTextLabel::m_szText);

    CGui::GetFontRender()->Render3DLabel(CGui::GetFontRender()->GetFontSize() - 3, pos, (char*)szText.c_str(), CTextLabel::m_dwColor);
}

void CTextLabel::RenderAttachedVehicle()
{
    // if (!CMultiPlayer::GetActiveNetwork()) {
    //     return;
    // }
    //
    // CVehiclePool* pVehiclePool = CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetVehiclePool();
    // if (!pVehiclePool) {
    //     return;
    // }
    //
    // CVehicle* pVehicle = pVehiclePool->GetVehicle(CTextLabel::m_attachedToVehicleID);
    // if (!pVehicle) {
    //     return;
    // }
    //
    // if (!pVehiclePool->IsValidPointer(CTextLabel::m_attachedToVehicleID, pVehicle)) {
    //     CNecessary::Log(OBFUSCATE("RenderAttachedVehicle: pVehicle is no longer valid."));
    //     return;
    // }
    //
    // if (!pVehicle->VerifyInstance()) {
    //     return;
    // }
    //
    // if (!pVehicle->IsAdded() || pVehicle->GetDistanceFromLocalPlayerPed() > CTextLabel::m_fDrawDistance) {
    //     return;
    // }
    //
    // CVector textPos;
    // CMatrix matVehicle;
    // pVehicle->GetMatrix(&matVehicle);
    // textPos.x = CTextLabel::m_vecOffsetCoords.x;
    // textPos.y = CTextLabel::m_vecOffsetCoords.y;
    // textPos.z = CTextLabel::m_vecOffsetCoords.z;
    //
    // CVector playerPos;
    // CCommon::ProjectMatrix(&playerPos, &matVehicle, &textPos);
    //
    // CVector Out;
    // CSprite::CalcScreenCoors(&playerPos, &Out, 0, 0, 0, 0);
    //
    // if (Out.z < 1.0f) {
    //     return;
    // }
    //
    // int hitEntity = 0;
    // if (CTextLabel::IsLosSceneUsing(&hitEntity, &playerPos))
    // {
    //     if (!hitEntity)
    //     {
    //         return;
    //     }
    // }
    //
    // ImVec2 pos(static_cast<int>(Out.x), static_cast<int>(Out.y));
    //
    // std::string szText(CTextLabel::m_szText);
    //
    // CGui::GetFontRender()->Render3DLabel(CGui::GetFontRender()->GetFontSize() - 3, pos, (char*)szText.c_str(), CTextLabel::m_dwColor);
}

void CTextLabel::RenderPos()
{
    CVector textPos;
    textPos.x = CTextLabel::m_vecPos.x;
    textPos.y = CTextLabel::m_vecPos.y;
    textPos.z = CTextLabel::m_vecPos.z;

    bool bDrawDist = CTextLabel::IsTargetDistance(textPos);
    if (!bDrawDist)
    {
        return;
    }

    CVector Out{};
    if (!RsGlobal || RsGlobal->maximumWidth <= 0 || RsGlobal->maximumHeight <= 0 || !CSprite::CalcScreenCoors(&textPos, &Out, nullptr, nullptr, false, false) || !std::isfinite(Out.x) || !std::isfinite(Out.y) || !std::isfinite(Out.z)) {
        return;
    }

    Out.x *= ImGui::GetIO().DisplaySize.x / static_cast<float>(RsGlobal->maximumWidth);
    Out.y *= ImGui::GetIO().DisplaySize.y / static_cast<float>(RsGlobal->maximumHeight);

    if (Out.z < 1.0f) {
        return;
    }

    int hitEntity = 0;
    if (CTextLabel::IsLosSceneUsing(&hitEntity, &textPos))
    {
        if (!hitEntity)
        {
            return;
        }
    }

    ImVec2 pos(static_cast<int>(Out.x), static_cast<int>(Out.y));

    std::string szText(CTextLabel::m_szText);

    CGui::GetFontRender()->Render3DLabel(CGui::GetFontRender()->GetFontSize() - 3, pos, (char*)szText.c_str(), CTextLabel::m_dwColor);
}

bool CTextLabel::IsLosSceneUsing(int* hitEntity, CVector* targetPos)
{
    if (!CTextLabel::m_bUseLineOfSight) {
        return false;
    }

    CAMERA_AIM *pCam = CCameraAim::GameGetInternalAim();

    *hitEntity = CScripting::ScriptCommand(&get_line_of_sight, targetPos->x, targetPos->y, targetPos->z, pCam->pos1x, pCam->pos1y, pCam->pos1z, 1, 0, 0, 1, 0);
    return true;
}