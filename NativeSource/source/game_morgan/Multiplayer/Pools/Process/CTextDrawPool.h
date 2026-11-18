#pragma once

#include "../../Guis/CTextDraw.h"
#include "../../../Enums/eTouchType.h"
#include "../../../Enums/eInvalidId.h"
#include "../../../Enums/eMaxItem.h"

class CTextDrawPool
{
private:
    CTextDraw*          m_pTextDraws[eMaxItem::MAX_TEXT_DRAWS];
    bool                m_bSelectState;
    uint32_t            m_dwHoverColor;
    uint16_t            m_wClickedTextDrawID;

public:
    CTextDrawPool();
    ~CTextDrawPool();
    void Process();

    CTextDraw* GetTextDraw(uint16_t id) {
        if (id >= eMaxItem::MAX_TEXT_DRAWS) {
            return (CTextDraw*)nullptr;
        }

        return m_pTextDraws[id];
    };

    bool GetTextDrawInSlot(uint16_t id) {
        return GetTextDraw(id) ? true : false;
    };

    bool GetState()
    {
        return m_bSelectState;
    };

    CTextDraw* Add(uint16_t wText, TEXT_DRAW_TRANSMIT *TextDrawTransmit, char *szText);
    void Delete(uint16_t id);

    void SetSelectState(bool bState, uint32_t dwColor);
    bool OnTouchEvent(eTouchType type, bool multi, int x, int y);
    void SendClickTextDraw();
    void DeleteAll();

    void Draw();
    void SnapshotProcess();
};