#include "../../../../../Main.h"
#include "CTextDrawPool.h"
#include "../../Guis/CGui.h"
#include "../../../CGame.h"
#include "../../../CCommon.h"
#include "../../CMultiPlayer.h"
#include "../../Core/RPCData/eRPCID.h"

#include <new>

CTextDrawPool::CTextDrawPool()
{
    CNecessary::Log(OBFUSCATE("CTextDrawPool Initialise"));
    for (int i = 0; i < eMaxItem::MAX_TEXT_DRAWS; i++)
    {
        CTextDrawPool::m_pTextDraws[i] = NULL;
    }

    CTextDrawPool::m_bSelectState = false;
    CTextDrawPool::m_dwHoverColor = 0;
    CTextDrawPool::m_wClickedTextDrawID = static_cast<uint16_t>(eInvalidId::INVALID_ID);

    // if (CGui::GetChat())
    // {
    //     CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("CTextDrawPool Initialise"));
    // }
}

CTextDrawPool::~CTextDrawPool()
{
    CNecessary::Log(OBFUSCATE("CTextDrawPool Uninitialise"));
    CTextDrawPool::DeleteAll();
}

void CTextDrawPool::DeleteAll()
{
    for (int i = 0; i < eMaxItem::MAX_TEXT_DRAWS; i++)
    {
        CTextDrawPool::Delete(i);
    }

    CTextDrawPool::m_bSelectState = false;
    CTextDrawPool::m_dwHoverColor = 0;
    CTextDrawPool::m_wClickedTextDrawID = static_cast<uint16_t>(eInvalidId::INVALID_ID);
}

CTextDraw* CTextDrawPool::Add(unsigned short wText, TEXT_DRAW_TRANSMIT *TextDrawTransmit, char *szText)
{
    if (wText >= eMaxItem::MAX_TEXT_DRAWS || !TextDrawTransmit) {
        return nullptr;
    }

    CTextDrawPool::Delete(wText);
    CTextDrawPool::m_pTextDraws[wText] = new (std::nothrow) CTextDraw(TextDrawTransmit, szText);
    if (CTextDrawPool::m_pTextDraws[wText])
    {
        return CTextDrawPool::m_pTextDraws[wText];
    }

    return nullptr;
}

void CTextDrawPool::Delete(unsigned short wText)
{
    if (!CTextDrawPool::GetTextDrawInSlot(wText)) {
        return;
    }

    SAFE_DELETE(CTextDrawPool::m_pTextDraws[wText]);
}

void CTextDrawPool::SetSelectState(bool bState, uint32_t dwColor)
{
    if (bState)
    {
        CGame::DisplayHUD(false);
        CTextDrawPool::m_bSelectState = true;
        CTextDrawPool::m_dwHoverColor = dwColor;
        CTextDrawPool::m_wClickedTextDrawID = static_cast<uint16_t>(eInvalidId::INVALID_ID);
    }
    else
    {
        CGame::DisplayHUD(true);
        CTextDrawPool::m_bSelectState = false;
        CTextDrawPool::m_dwHoverColor = 0;
        CTextDrawPool::m_wClickedTextDrawID = static_cast<uint16_t>(eInvalidId::INVALID_ID);

        CTextDrawPool::SendClickTextDraw();
        for (int i = 0; i < eMaxItem::MAX_TEXT_DRAWS; i++)
        {
            if (CTextDrawPool::m_pTextDraws[i] && CTextDrawPool::GetTextDrawInSlot(i))
            {
                CTextDrawPool::m_pTextDraws[i]->m_bHovered = false;
                CTextDrawPool::m_pTextDraws[i]->m_dwHoverColor = 0;
            }
        }
    }
}

bool CTextDrawPool::OnTouchEvent(eTouchType type, [[maybe_unused]] bool multi, int x, int y)
{
    if (!CTextDrawPool::GetState()) {
        return true;
    }

    CGame::DisplayHUD(false);

    CTextDrawPool::m_wClickedTextDrawID = static_cast<uint16_t>(eInvalidId::INVALID_ID);
    for (int i = 0; i < eMaxItem::MAX_TEXT_DRAWS; i++)
    {
        if (CTextDrawPool::m_pTextDraws[i] && CTextDrawPool::GetTextDrawInSlot(i) && CTextDrawPool::m_pTextDraws[i]->m_TextDrawData.byteSelectable && CTextDrawPool::m_pTextDraws[i]->m_TextDrawData.bHasRectArea)
        {
            CTextDrawPool::m_pTextDraws[i]->m_bHovered = false;
            CTextDrawPool::m_pTextDraws[i]->m_dwHoverColor = 0;

            if (CCommon::IsPointInRect(x, y, &CTextDrawPool::m_pTextDraws[i]->m_rectArea))
            {
                switch (type)
                {
                    case eTouchType::TOUCH_PUSH:
                    case eTouchType::TOUCH_MOVE: {
                        CTextDrawPool::m_wClickedTextDrawID = static_cast<uint16_t>(eInvalidId::INVALID_ID);
                        CTextDrawPool::m_pTextDraws[i]->m_bHovered = true;
                        CTextDrawPool::m_pTextDraws[i]->m_dwHoverColor = m_dwHoverColor;

                        break;
                    }

                    case eTouchType::TOUCH_POP: {
                        CTextDrawPool::m_wClickedTextDrawID = i;
                        CTextDrawPool::SendClickTextDraw();
                        CTextDrawPool::m_pTextDraws[i]->m_bHovered = false;
                        CTextDrawPool::m_pTextDraws[i]->m_dwHoverColor = 0;

                        break;
                    }
                }
            }
        }
    }

    return false;
}

void CTextDrawPool::SendClickTextDraw()
{
    auto* network = CMultiPlayer::GetActiveNetwork();
    if (!network || !network->GetRakClient()) {
        return;
    }

    RakNet::BitStream bsSend;
    bsSend.Write(m_wClickedTextDrawID);
    int rpcId = RPC_ClickTextdraw;
    network->GetRakClient()->RPC(&rpcId, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CTextDrawPool::Draw()
{
    for (int i = 0; i < eMaxItem::MAX_TEXT_DRAWS; i++)
    {
        if (CTextDrawPool::m_pTextDraws[i] && CTextDrawPool::GetTextDrawInSlot(i))
        {
            CTextDrawPool::m_pTextDraws[i]->Draw();
        }
    }
}

void CTextDrawPool::SnapshotProcess()
{
    for (int i = 0; i < eMaxItem::MAX_TEXT_DRAWS; i++)
    {
        if (CTextDrawPool::m_pTextDraws[i] && CTextDrawPool::GetTextDrawInSlot(i))
        {
            CTextDrawPool::m_pTextDraws[i]->SnapshotProcess();
        }
    }
}