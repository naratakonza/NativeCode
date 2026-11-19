#pragma once

#include <cstdint>

#include "../../Core/CVector.h"
#include "../../Enums/eInvalidId.h"

class CTextLabel
{
public:
    uint16_t    m_id;
    char        m_szText[2048 + 1] = { '\0' };
    char        m_szTextWithoutColors[2048 + 1] = { '\0' };
    uint32_t    m_dwColor;
    CVector     m_vecPos;
    float       m_fDrawDistance;
    int         m_iVirtualWorld;
    uint8_t     m_bUseLineOfSight;
    uint16_t    m_attachedToPlayerID;
    uint16_t    m_attachedToVehicleID;
    CVector     m_vecOffsetCoords;
    float m_fTrueX;

public:
    CTextLabel(uint16_t id, char* text, uint32_t color, CVector vecPos, float drawDistance, uint8_t useLOS, uint16_t attachedToPlayerID, uint16_t attachedToVehicleID);
    ~CTextLabel();

    void Render();
    void Disable();
    bool IsTargetDistance(CVector f);
    void RenderPos();
    void RenderAttachedPlayer();
    void RenderAttachedVehicle();
    bool IsLosSceneUsing(int* hitEntity, CVector* targetPos);
};