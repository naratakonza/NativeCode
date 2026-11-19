#include "../../../../Main.h"
#include "CTextDraw.h"
#include "../../CGame.h"
#include "../../CFont.h"
#include "../../CUtil.h"
#include "../../CCommon.h"
#include "../../CTxdManager.h"
#include "../../Enums/eFontAlignment.h"
#include "../Snapshot/CModelSnapshot.h"


CTextDraw::CTextDraw(TEXT_DRAW_TRANSMIT *pTextDrawTransmit, const char *szText)
{
    memset(&m_TextDrawData, 0, sizeof(TEXT_DRAW_DATA));

    CTextDraw::m_TextDrawData.fLetterWidth = pTextDrawTransmit->fLetterWidth;
    CTextDraw::m_TextDrawData.fLetterHeight = pTextDrawTransmit->fLetterHeight;

    CTextDraw::m_TextDrawData.dwLetterColor = pTextDrawTransmit->dwLetterColor;
    CTextDraw::m_TextDrawData.byteUnk12 = 0;
    CTextDraw::m_TextDrawData.byteCentered = pTextDrawTransmit->byteCenter;
    CTextDraw::m_TextDrawData.byteBox = pTextDrawTransmit->byteBox;

    CTextDraw::m_TextDrawData.fLineWidth = pTextDrawTransmit->fLineWidth;
    CTextDraw::m_TextDrawData.fLineHeight = pTextDrawTransmit->fLineHeight;

    CTextDraw::m_TextDrawData.dwBoxColor = pTextDrawTransmit->dwBoxColor;
    CTextDraw::m_TextDrawData.byteProportional = pTextDrawTransmit->byteProportional;
    CTextDraw::m_TextDrawData.dwBackgroundColor = pTextDrawTransmit->dwBackgroundColor;
    CTextDraw::m_TextDrawData.byteShadow = pTextDrawTransmit->byteShadow;
    CTextDraw::m_TextDrawData.byteOutline = pTextDrawTransmit->byteOutline;
    CTextDraw::m_TextDrawData.byteAlignLeft = pTextDrawTransmit->byteLeft;
    CTextDraw::m_TextDrawData.byteAlignRight = pTextDrawTransmit->byteRight;
    CTextDraw::m_TextDrawData.dwStyle = pTextDrawTransmit->byteStyle;

    CTextDraw::m_TextDrawData.fX = pTextDrawTransmit->fX;
    CTextDraw::m_TextDrawData.fY = pTextDrawTransmit->fY;

    CTextDraw::m_TextDrawData.dwParam1 = 0xFFFFFFFF;
    CTextDraw::m_TextDrawData.dwParam2 = 0xFFFFFFFF;
    CTextDraw::m_TextDrawData.byteSelectable = pTextDrawTransmit->byteSelectable;
    CTextDraw::m_TextDrawData.wModelID = pTextDrawTransmit->wModelID;
    CTextDraw::m_TextDrawData.vecRot.x = pTextDrawTransmit->vecRot.x;
    CTextDraw::m_TextDrawData.vecRot.y = pTextDrawTransmit->vecRot.y;
    CTextDraw::m_TextDrawData.vecRot.z = pTextDrawTransmit->vecRot.z;
    CTextDraw::m_TextDrawData.fZoom = pTextDrawTransmit->fZoom;
    CTextDraw::m_TextDrawData.wColor1 = pTextDrawTransmit->wColor1;
    CTextDraw::m_TextDrawData.wColor2 = pTextDrawTransmit->wColor2;
    CTextDraw::m_TextDrawData.bHasKeyCode = false;
    CTextDraw::m_TextDrawData.iTextureSlot = -1;
    CTextDraw::SetText(szText);

    if (CTextDraw::m_TextDrawData.dwStyle == eTextDrawStyle::TEXTDRAW_TXD_SPRITE)
    {
        CTextDraw::m_TextDrawData.iTextureSlot = CCommon::GetFreeTextDrawTextureSlot();
        CTextDraw::LoadTexture();
    }

    CTextDraw::m_TextDrawData.bHasRectArea = false;
    CTextDraw::m_rectArea.X1 = 0.0f;
    CTextDraw::m_rectArea.Y1 = 0.0f;
    CTextDraw::m_rectArea.X2 = 0.0f;
    CTextDraw::m_rectArea.Y2 = 0.0f;
    CTextDraw::m_bHovered = false;
    CTextDraw::m_dwHoverColor = 0;
}

CTextDraw::~CTextDraw()
{
    CCommon::DestroyTextDrawTexture(CTextDraw::m_TextDrawData.iTextureSlot);
    CTextDraw::m_rectArea.X1 = 0.0f;
    CTextDraw::m_rectArea.Y1 = 0.0f;
    CTextDraw::m_rectArea.X2 = 0.0f;
    CTextDraw::m_rectArea.Y2 = 0.0f;
    CTextDraw::m_bHovered = false;
    CTextDraw::m_dwHoverColor = 0;

    memset(&m_TextDrawData, 0, sizeof(TEXT_DRAW_DATA));
}

void CTextDraw::Draw()
{
    if (CTextDraw::m_TextDrawData.iTextureSlot != -1 || CTextDraw::m_TextDrawData.dwStyle == eTextDrawStyle::TEXTDRAW_TXD_SPRITE)
    {
        CTextDraw::DrawTextured();
    }
    else
    {
        CTextDraw::DrawDefault();
    }
}

void CTextDraw::DrawTextured()
{
    const CRect screenArea = CGame::GetScreenSafeArea();
    if (screenArea.X2 <= screenArea.X1 || screenArea.Y2 <= screenArea.Y1) {
        return;
    }

    float scaleX = (screenArea.X2 - screenArea.X1) * (1.0f / 640.0f);
    float scaleY = (screenArea.Y2 - screenArea.Y1) * (1.0f / 448.0f);

    CTextDraw::m_rectArea.X1 = screenArea.X1 + CTextDraw::m_TextDrawData.fX * scaleX;
    CTextDraw::m_rectArea.Y1 = screenArea.Y1 + CTextDraw::m_TextDrawData.fY * scaleY;
    CTextDraw::m_rectArea.X2 = screenArea.X1 + (CTextDraw::m_TextDrawData.fX + CTextDraw::m_TextDrawData.fLineWidth) * scaleX;
    CTextDraw::m_rectArea.Y2 = screenArea.Y1 + (CTextDraw::m_TextDrawData.fY + CTextDraw::m_TextDrawData.fLineHeight) * scaleY;

    static float uv_reflected[8] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    static float uv_normal[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    CRect rect;
    rect.X1 = CTextDraw::m_rectArea.X1;
    rect.Y1 = CTextDraw::m_rectArea.Y2;
    rect.X2 = CTextDraw::m_rectArea.X2;
    rect.Y2 = CTextDraw::m_rectArea.Y1;

    CTextDraw::m_TextDrawData.bHasRectArea = true;

    const auto texture = CCommon::pTextDrawTexture.find(CTextDraw::m_TextDrawData.iTextureSlot);
    if (texture == CCommon::pTextDrawTexture.end()) {
        return;
    }

    if (CTextDraw::m_bHovered)
    {
        uint32_t dwReversed = __builtin_bswap32(CTextDraw::m_dwHoverColor | (0x000000FF));
        CCommon::DrawTextureUV(texture->second, &rect, dwReversed, CTextDraw::m_TextDrawData.dwStyle == eTextDrawStyle::TEXTDRAW_MODEL_PREVIEW ? uv_reflected : uv_normal);
    }
    else
    {
        CCommon::DrawTextureUV(texture->second, &rect, CTextDraw::m_TextDrawData.dwLetterColor, CTextDraw::m_TextDrawData.dwStyle == eTextDrawStyle::TEXTDRAW_MODEL_PREVIEW ? uv_reflected : uv_normal);
    }
}

void CTextDraw::DrawDefault()
{
    if (!CTextDraw::m_szText[0] || CTextDraw::m_TextDrawData.dwStyle == eTextDrawStyle::TEXTDRAW_MODEL_PREVIEW || CTextDraw::m_TextDrawData.dwStyle == eTextDrawStyle::TEXTDRAW_TXD_SPRITE) {
        return;
    }

    strcpy(CTextDraw::m_szString, CTextDraw::m_szText);

    const CRect screenArea = CGame::GetScreenSafeArea();
    float fScreenWidth = screenArea.X2 - screenArea.X1;
    float fScreenHeight = screenArea.Y2 - screenArea.Y1;
    if (fScreenWidth <= 0.0f || fScreenHeight <= 0.0f) {
        return;
    }

    float fHorizHudScale = 1.0f / 640.0f;
    float fVertHudScale = 1.0f / 448.0f;

    float fScaleY = (float)fScreenHeight * fVertHudScale * CTextDraw::m_TextDrawData.fLetterHeight * 0.5f;
    float fScaleX = (float)fScreenWidth * fHorizHudScale * CTextDraw::m_TextDrawData.fLetterWidth;

    CFont::SetScale(fScaleX, fScaleY);

    if (CTextDraw::m_bHovered)
    {
        uint32_t dwReversed = __builtin_bswap32(CTextDraw::m_dwHoverColor | (0x000000FF));
        CFont::SetColor(&dwReversed);
    }
    else
    {
        uint32_t dwLetterColor = CTextDraw::m_TextDrawData.dwLetterColor;
        CFont::SetColor(&dwLetterColor);
    }

    CFont::SetJustify(0);

    if (CTextDraw::m_TextDrawData.byteAlignRight)
    {
        CFont::SetOrientation(eFontAlignment::ALIGN_RIGHT);
    }
    else if (CTextDraw::m_TextDrawData.byteCentered)
    {
        CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);
    }
    else
    {
        CFont::SetOrientation(eFontAlignment::ALIGN_LEFT);
    }

    float fLineWidth = fScreenWidth * CTextDraw::m_TextDrawData.fLineWidth * fHorizHudScale;
    CFont::SetWrapx(screenArea.X1 + fLineWidth);

    float fLineHeight = fScreenWidth * CTextDraw::m_TextDrawData.fLineHeight * fHorizHudScale;
    CFont::SetCentreSize(fLineHeight);

    CFont::SetBackground(CTextDraw::m_TextDrawData.byteBox, 0);
    uint32_t dwBoxColor = CTextDraw::m_TextDrawData.dwBoxColor;
    CFont::SetBackgroundColor(&dwBoxColor);
    CFont::SetProportional(CTextDraw::m_TextDrawData.byteProportional);
    uint32_t dwBackgroundColor = CTextDraw::m_TextDrawData.dwBackgroundColor;
    CFont::SetDropColor(&dwBackgroundColor);

    if (CTextDraw::m_TextDrawData.byteOutline)
    {
        CFont::SetEdge(CTextDraw::m_TextDrawData.byteOutline);
    }
    else
    {
        CFont::SetDropShadowPosition(CTextDraw::m_TextDrawData.byteShadow);
    }

    CFont::SetFontStyle(static_cast<uint8_t>(CTextDraw::m_TextDrawData.dwStyle));

    float fUseY = screenArea.Y1 + fScreenHeight - ((448.0 - CTextDraw::m_TextDrawData.fY) * (fScreenHeight * fVertHudScale));
    float fUseX = screenArea.X1 + fScreenWidth - ((640.0 - CTextDraw::m_TextDrawData.fX) * (fScreenWidth * fHorizHudScale));

    std::string strString = CUtil::ReplaceKey(std::string(CTextDraw::m_szString));

    if (!CTextDraw::m_TextDrawData.byteBox && CUtil::HasNewlineTag(strString))
    {
        CUtil::DrawMultiLineTextDraw(fUseX, fUseY, strString);
    }
    else
    {
        strString = CUtil::ProcessTextDrawString(strString);
        CFont::PrintString(fUseX, fUseY, reinterpret_cast<const char*>(strString.c_str()));
    }

    CFont::SetEdge(0);

    if (CTextDraw::m_TextDrawData.byteAlignRight)
    {
        CTextDraw::m_rectArea.X1 = fUseX - (fLineWidth - (fUseX - screenArea.X1));
        CTextDraw::m_rectArea.X2 = fUseX;
        CTextDraw::m_rectArea.Y1 = fUseY;
        CTextDraw::m_rectArea.Y2 = fUseY + fLineHeight;
    }
    else if (CTextDraw::m_TextDrawData.byteCentered)
    {
        CTextDraw::m_rectArea.X1 = fUseX - (fLineHeight * 0.5f);
        CTextDraw::m_rectArea.X2 = CTextDraw::m_rectArea.X1 + fLineHeight;
        CTextDraw::m_rectArea.Y1 = fUseY;
        CTextDraw::m_rectArea.Y2 = fUseY + fLineWidth;
    }
    else
    {
        CTextDraw::m_rectArea.X1 = fUseX;
        CTextDraw::m_rectArea.X2 = screenArea.X1 + fLineWidth;
        CTextDraw::m_rectArea.Y1 = fUseY;
        CTextDraw::m_rectArea.Y2 = fUseY + fLineHeight;
    }

    CTextDraw::m_TextDrawData.bHasRectArea = true;
}

void CTextDraw::SetText(const char *szText)
{
    memset(CTextDraw::m_szText, 0, eMaxItem::MAX_TEXT_DRAW_LINE);

    if (!szText || strnlen(szText, eMaxItem::MAX_TEXT_DRAW_LINE) >= eMaxItem::MAX_TEXT_DRAW_LINE) {
        return;
    }

    strncpy(CTextDraw::m_szText, szText, eMaxItem::MAX_TEXT_DRAW_LINE);
    CTextDraw::m_szText[eMaxItem::MAX_TEXT_DRAW_LINE] = '\0';

    if (CTextDraw::m_TextDrawData.dwStyle == eTextDrawStyle::TEXTDRAW_TXD_SPRITE && CTextDraw::m_TextDrawData.iTextureSlot != -1)
    {
        CCommon::DestroyTextDrawTexture(CTextDraw::m_TextDrawData.iTextureSlot);
        CTextDraw::m_TextDrawData.iTextureSlot = CCommon::GetFreeTextDrawTextureSlot();
        CTextDraw::LoadTexture();
    }
}

void CTextDraw::LoadTexture()
{
    char szTxdName[64 + 1];
    char szTextureName[64 + 1];

    memset(szTxdName, 0, sizeof(szTxdName));
    memset(szTextureName, 0, sizeof(szTextureName));

    char *szTextureDelimenter = strchr(CTextDraw::m_szText, ':');
    if (szTextureDelimenter)
    {
        if (CNecessary::MPSTRLEN(CTextDraw::m_szText) <= 64 && !strchr(CTextDraw::m_szText, '\\') && !strchr(CTextDraw::m_szText, '/'))
        {
            strncpy(szTxdName, CTextDraw::m_szText, (size_t)(szTextureDelimenter - CTextDraw::m_szText));
            strcpy(szTextureName, ++szTextureDelimenter);

            if (m_TextDrawData.iTextureSlot != -1)
            {
                bool bTxdInSlot = false;
                if (!strcasecmp(szTxdName, OBFUSCATE("hud")))
                {
                    bTxdInSlot = true;
                }
                else
                {
                    bTxdInSlot = CTxdManager::LoadTxdSlotIfNeeded(szTxdName);
                }

                if (!strcasecmp(szTxdName, OBFUSCATE("vehicleprev")))
                {
                    if(!bTxdInSlot)
                    {
                        bTxdInSlot = CTxdManager::LoadTxdSlotIfNeeded(OBFUSCATE("vehicles128"));
                    }
                }

                if (bTxdInSlot)
                {
                    CCommon::pTextDrawTexture[CTextDraw::m_TextDrawData.iTextureSlot] = CTxdManager::LoadTextureFromTxd(reinterpret_cast<const char*>(szTxdName), reinterpret_cast<const char*>(szTextureName));
                }
            }
        }
    }
}

void CTextDraw::SnapshotProcess()
{
    if (CTextDraw::m_TextDrawData.dwStyle != eTextDrawStyle::TEXTDRAW_MODEL_PREVIEW || CTextDraw::m_TextDrawData.iTextureSlot != -1) {
        return;
    }

    RwTexture *pSnapshotProcessed = nullptr;
    if (CCommon::IsPedModel(CTextDraw::m_TextDrawData.wModelID))
    {
        if (!CCommon::GetModelInfoByID(CTextDraw::m_TextDrawData.wModelID))
        {
            CTextDraw::m_TextDrawData.wModelID = 0;
        }

        pSnapshotProcessed = CGame::GetModelSnapshot()->CreatePedSnapShot(CTextDraw::m_TextDrawData.wModelID, CTextDraw::m_TextDrawData.dwBackgroundColor, &m_TextDrawData.vecRot, CTextDraw::m_TextDrawData.fZoom);
    }
    /*
    else if (CTextDraw::m_TextDrawData.wModelID >= 400 && CTextDraw::m_TextDrawData.wModelID <= 611)
    {
        pSnapshotProcessed = CGame::GetModelSnapshot()->CreateVehicleSnapShot(CTextDraw::m_TextDrawData.wModelID, CTextDraw::m_TextDrawData.dwBackgroundColor, &m_TextDrawData.vecRot, CTextDraw::m_TextDrawData.fZoom, CTextDraw::m_TextDrawData.wColor1, CTextDraw::m_TextDrawData.wColor2);
    }
    else if ((CTextDraw::m_TextDrawData.wModelID <= 320 || CTextDraw::m_TextDrawData.wModelID > 373) && CTextDraw::m_TextDrawData.wModelID < 615)
    {
        if (CTextDraw::m_TextDrawData.wModelID >= 0) {
            return;
        }

        if (!CCommon::IsExistForModelInfo(CTextDraw::m_TextDrawData.wModelID))
        {
            CTextDraw::m_TextDrawData.wModelID = eSpecialObject::OBJECT_NOMODELFILE;
        }

        pSnapshotProcessed = CGame::GetModelSnapshot()->CreateObjectSnapShot(CTextDraw::m_TextDrawData.wModelID, CTextDraw::m_TextDrawData.dwBackgroundColor, &m_TextDrawData.vecRot, CTextDraw::m_TextDrawData.fZoom);
    }
    else
    {
        if (!CCommon::GetModelInfoByID(CTextDraw::m_TextDrawData.wModelID))
        {
            CTextDraw::m_TextDrawData.wModelID = eSpecialObject::OBJECT_NOMODELFILE;
        }

        pSnapshotProcessed = CGame::GetModelSnapshot()->CreateObjectSnapShot(CTextDraw::m_TextDrawData.wModelID, CTextDraw::m_TextDrawData.dwBackgroundColor, &m_TextDrawData.vecRot, CTextDraw::m_TextDrawData.fZoom);
    }
    */

    if (pSnapshotProcessed)
    {
        CTextDraw::m_TextDrawData.iTextureSlot = CCommon::GetFreeTextDrawTextureSlot();
        CCommon::pTextDrawTexture[CTextDraw::m_TextDrawData.iTextureSlot] = pSnapshotProcessed;
    }
}