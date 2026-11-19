#include "../../../../../Main.h"
#include "CFontRender.h"
#include "../CGui.h"
#include "../../CMultiPlayer.h"
#include "../../../CUtil.h"
#include "../../../../utils/Storage/CStorage.h"
#include "../../../../vendor/imgui/imgui_impl_renderware.h"

#include <CoreGraphics/CoreGraphics.h>

static constexpr float atlasFontSize = 30.0f;

CFontRender::CFontRender()
{
    CNecessary::Log(OBFUSCATE("Initializing CFontRender.."));

    ImGuiContext* previous = ImGui::GetCurrentContext();
    IMGUI_CHECKVERSION();
    CFontRender::m_pContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(CFontRender::m_pContext);
    ImGuiIO &io = ImGui::GetIO();

    CFontRender::m_bBackendReady = ImGui_ImplRenderWare_Init();
    if (!CFontRender::m_bBackendReady)
    {
        CNecessary::Log(OBFUSCATE("CFontRender | RenderWare bindings unavailable"));
    }

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // setup style
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    ImGui::StyleColorsDark();

    // load font
    const auto fontName = OBFUSCATE("arial.ttf");
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF,
        0x0100, 0x017F,
        0x0400, 0x04FF,
        0x0E00, 0x0E5B,
        0x2DE0, 0x2DFF,
        0xA640, 0xA69F,
        0xF020, 0xF0FF,
        0x2010, 0x205E,
        0
    };

    io.Fonts->TexDesiredWidth = 2048;
    CNecessary::Log(OBFUSCATE("CFontRender | Loading font: %s"), static_cast<const char*>(fontName));
    io.FontDefault = CFontRender::LoadFont(fontName, atlasFontSize, ranges);
    CNecessary::Log(OBFUSCATE("CFontRender | ImFont pointer = %p"), static_cast<void*>(io.FontDefault));
    if (!io.FontDefault)
    {
        CNecessary::Log(OBFUSCATE("CFontRender | Could not load SAMP/fonts/arial.ttf"));
        CFontRender::m_bBackendReady = false;
    }

    m_pFont = io.FontDefault;
    m_pFontGTAWeap = CFontRender::LoadFont(OBFUSCATE("gtaweap3.ttf"), atlasFontSize);
    m_pSampFont = CFontRender::LoadFont(OBFUSCATE("sampaux3.ttf"), atlasFontSize * 25.0f / 30.0f);
    if (!m_pFontGTAWeap) m_pFontGTAWeap = m_pFont;
    if (!m_pSampFont) m_pSampFont = m_pFont;

    style.WindowRounding = 0.0f;
    ImGui::SetCurrentContext(previous);
}

CFontRender::~CFontRender()
{
    CNecessary::Log(OBFUSCATE("CFontRender Uninitialise"));

    ImGuiContext* previous = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(CFontRender::m_pContext);
    ImGui_ImplRenderWare_ShutDown();
    ImGui::DestroyContext(CFontRender::m_pContext);

    if (previous != CFontRender::m_pContext)
    {
        ImGui::SetCurrentContext(previous);
    }
}

void CFontRender::Render()
{
    if (!CFontRender::m_bBackendReady) {
        return;
    }

    ImGuiContext* previous = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(CFontRender::m_pContext);
    if (!ImGui_ImplRenderWare_NewFrame())
    {
        ImGui::SetCurrentContext(previous);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    auto* settings = CSetting::GetConfig();
    const float size = settings ? settings->GetFloatValue(OBFUSCATE("FontSize")) : atlasFontSize;
    const float fontSize = std::isfinite(size) ? std::clamp(size, 1.0f, 80.0f) : atlasFontSize;
    m_fFontSize = SCALE_Y(io, fontSize);
    io.FontGlobalScale = SCALE_Y(io, fontSize) / atlasFontSize;

    if (!CFontRender::m_dwLastFrameTick)
    {
        CNecessary::Log(OBFUSCATE("CFontRender | Scale factor: %f, %f Font size: %f"), io.DisplaySize.x * MULT_X, io.DisplaySize.y * MULT_Y, SCALE_Y(io, fontSize));
    }

    const auto now = CNecessary::GetTickCount();
    io.DeltaTime = CFontRender::m_dwLastFrameTick ? std::max(0.001f, (now - CFontRender::m_dwLastFrameTick) / 1000.0f) : 1.0f / 60.0f;
    CFontRender::m_dwLastFrameTick = now;

    CFontRender::PreProcessInput();
    ImGui::NewFrame();

    if (CGui::GetGPSMarker())
    {
        CGui::GetGPSMarker()->Render();
    }

    if (CGui::GetGPSNavigator())
    {
        CGui::GetGPSNavigator()->Render();
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextLabelPool())
    {
        CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextLabelPool()->Draw();
    }

	if (CGui::GetPlayerTag())
    {
        CGui::GetPlayerTag()->Render();
    }

    if (CGui::GetDebug())
    {
        CGui::GetDebug()->RenderTitle();
        CGui::GetDebug()->RenderAudioStreamTag();
    }

    if (CGui::GetButton())
    {
        CGui::GetButton()->Render();
    }

    if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->GetState())
    {
        if (CGui::GetXSing())
        {
            CGui::GetXSing()->Render();
        }
    }
    else
    {
        // เดียวมาทำต่อภายหลัง
        // if (CGui::GetGButton())
        // {
        //     CGui::GetGButton()->Render();
        // }
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());
    CFontRender::PostProcessInput();

    ImGui::SetCurrentContext(previous);
}

bool CFontRender::OnTouchEvent(eTouchType type, ImVec2 pos)
{
    // if (CGui::GetObjectSelection() && !CGui::GetObjectSelection()->OnTouchEvent(type, pos)) {
    //     return false;
    // }

    // if (CGui::GetObjectEdit() && !CGui::GetObjectEdit()->OnTouchEvent(type, pos)) {
    //     return false;
    // }

    // if (!CPlugin::OnTouchEvent(type, pos)) {
    //     return false;
    // }

    switch (type)
    {
        case eTouchType::TOUCH_PUSH:
        {
            m_vTouchPos = pos;
            m_bMouseDown = true;
            m_bNeedClearMousePos = false;
            m_bNextClear = false;

            break;
        }

        case eTouchType::TOUCH_POP:
        {
            m_vTouchPos = pos;
            m_bMouseDown = false;
            m_bNeedClearMousePos = true;

            break;
        }

        case eTouchType::TOUCH_MOVE:
        {
            m_vTouchPos = pos;
            m_bMouseDown = true;
            m_bNeedClearMousePos = false;

            break;
        }

        default: {
            return false;
        }
    }

    return true;
}

void CFontRender::PreProcessInput()
{
    ImGuiIO& io = ImGui::GetIO();

    if (m_bNextClear)
    {
        io.MousePos = ImVec2(-1.0f, -1.0f);
    }
    else
    {
        io.MousePos = m_vTouchPos;
    }

    io.MouseDown[0] = m_bMouseDown;
}

void CFontRender::PostProcessInput()
{
    if (m_bNeedClearMousePos && !m_bMouseDown)
    {
        m_bNextClear = true;
        m_bNeedClearMousePos = false;
    }
}

ImFont* CFontRender::LoadFont(const char* font, float fontsize, const ImWchar* ranges)
{
    ImGuiIO &io = ImGui::GetIO();

    ImFontConfig fontCfg;
    fontCfg.SizePixels = fontsize;
    fontCfg.OversampleH = 1;
    fontCfg.OversampleV = 1;

    char path[0xFF];
    std::snprintf(path, sizeof(path), OBFUSCATE("SAMP/fonts/%s"), font);
    auto result = morgan::fileio::CStorage::OpenActiveResource(nullptr, morgan::fileio::ResolveGameAssetPath(path));
    if (!result.file)
    {
        CNecessary::Log(OBFUSCATE("CFontRender | Font open failed: %s (error %d)"), path, result.error.value());
        return nullptr;
    }
    if (std::fseek(result.file.get(), 0, SEEK_END) != 0) {
        return nullptr;
    }

    const long size = std::ftell(result.file.get());
    if (size < 12 || size > 4 * 1024 * 1024 || std::fseek(result.file.get(), 0, SEEK_SET) != 0) {
        return nullptr;
    }

    auto* data = static_cast<unsigned char*>(ImGui::MemAlloc(static_cast<std::size_t>(size)));
    if (data)
    {
        const auto length = static_cast<std::size_t>(size);
        if (std::fread(data, 1, length, result.file.get()) == length && (std::memcmp(data, "\x00\x01\x00\x00", 4) == 0 || std::memcmp(data, "true", 4) == 0))
        {
            CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr, data, length, nullptr);
            CGFontRef validated = provider ? CGFontCreateWithDataProvider(provider) : nullptr;
            if (validated) CGFontRelease(validated);
            if (provider) CGDataProviderRelease(provider);

            if (validated)
            {
                return io.Fonts->AddFontFromMemoryTTF(data, static_cast<int>(size), fontsize, &fontCfg, ranges);
            }
        }

        ImGui::MemFree(data);
    }

    return nullptr;
}

void CFontRender::RenderText(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float fontSize)
{
    auto* settings = CSetting::GetConfig();
    float offset = settings ? std::clamp(settings->GetIntValue(OBFUSCATE("FontOutline")), 0, 8) : 2;
    if (fontSize > 0.0f)
    {
        offset = std::min(offset, fontSize / 12.0f);
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (bOutline && offset > 0.0f)
    {
        // left
        drawList->AddText(nullptr, fontSize, ImVec2(posCur.x - offset, posCur.y), IM_COL32_BLACK, text_begin, text_end);
        // right
        drawList->AddText(nullptr, fontSize, ImVec2(posCur.x + offset, posCur.y), IM_COL32_BLACK, text_begin, text_end);
        // above
        drawList->AddText(nullptr, fontSize, ImVec2(posCur.x, posCur.y - offset), IM_COL32_BLACK, text_begin, text_end);
        // below
        drawList->AddText(nullptr, fontSize, ImVec2(posCur.x, posCur.y + offset), IM_COL32_BLACK, text_begin, text_end);
    }

    drawList->AddText(nullptr, fontSize, posCur, col, text_begin, text_end);
}
bool CFontRender::IsColorDark(ImU32 col)
{
    // Extract RGB components from the color
    int r = (col >> 16) & 0xFF;
    int g = (col >> 8) & 0xFF;
    int b = col & 0xFF;

    // Calculate the brightness of the color
    double brightness = (r * 0.299 + g * 0.587 + b * 0.114) / 255.0;

    // Log::print("brightness dark: %.2f", brightness);

    // Return true if the color is dark
    return brightness >= 1.0;
}

void CFontRender::RenderTextMethod(ImVec2& posCur, ImU32 col, bool bOutline, const char* text_begin, const char* text_end, float font_size, ImFont *font, bool bOutlineUseTextColor)
{
    if (bOutline && CSetting::GetConfig()->GetIntValue(OBFUSCATE("FontOutline")) > 0)
    {
        const int offset = std::clamp(CSetting::GetConfig()->GetIntValue(OBFUSCATE("FontOutline")), 0, 8);
        for (int x = -offset; x <= offset; x += offset)
        {
            for (int y = -offset; y <= offset; y += offset)
            {
                if (x == 0 && y == 0) continue;

                if (!CFontRender::IsColorDark(col))
                {
                    ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? CGui::GetFontRender()->GetFontSize() : font_size, ImVec2(posCur.x + x, posCur.y + y), bOutlineUseTextColor ? ImColor(col) : ImColor(IM_COL32_BLACK), text_begin, text_end);
                }
                else
                {
                    // ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? fontSize : font_size, ImVec2(posCur.x + x, posCur.y + y), bOutlineUseTextColor ? ImColor(col) : ImColor(0xFFFAFAFA), text_begin, text_end);
                }
            }
        }
    }

    ImGui::GetBackgroundDrawList()->AddText(font, font_size == 0.0f ? CGui::GetFontRender()->GetFontSize() : font_size, posCur, col, text_begin, text_end);
}

ImVec2 CFontRender::CalcTextSizeWithoutTags(char* szStr)
{
    if (!szStr) {
        return ImVec2(0, 0);
    }

    char szNonColored[2048 + 1];
    int iNonColoredMsgLen = 0;

    for (std::size_t pos = 0; pos < CNecessary::MPSTRLEN(szStr) && szStr[pos] != '\0'; pos++)
    {
        if (szStr[pos] == '{' && pos + 7 < CNecessary::MPSTRLEN(szStr))
        {
            ImVec4 tempColor;
            if ((szStr[pos + 7] == '}' && CGui::GetFontRender()->ProcessInlineHexColor(&szStr[pos + 1], &szStr[pos + 7], tempColor)) || (pos + 9 < CNecessary::MPSTRLEN(szStr) && szStr[pos + 9] == '}' && CGui::GetFontRender()->ProcessInlineHexColor(&szStr[pos + 1], &szStr[pos + 9], tempColor)))
            {
                pos += (szStr[pos + 7] == '}' ? 7 : 9);
                continue;
            }
        }

        if (iNonColoredMsgLen < 2048)
        {
            szNonColored[iNonColoredMsgLen] = szStr[pos];
            iNonColoredMsgLen++;
        }
    }

    szNonColored[iNonColoredMsgLen] = 0;

    return ImGui::CalcTextSize(szNonColored);
}

void CFontRender::TextWithColorsW(ImFont *font, const float font_size, uint8_t outline, ImVec2 pos, ImColor col, const char* szStr)
{
    if (!CMultiPlayer::GetActiveNetwork() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer() || !CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()) {
        return;
    }

    char tempStr[4096] = { '\0' };
    ImVec2 vecPos = pos;
    std::snprintf(tempStr, sizeof(tempStr), "%s", szStr);
    tempStr[sizeof(tempStr) - 1] = '\0';

    const char* textStart = tempStr;
    const char* textCur = tempStr;

    while (textCur < (tempStr + sizeof(tempStr)) && *textCur != '\0')
    {
        if (*textCur == '{')
        {
            if (textCur != textStart)
            {
                CGui::GetFontRender()->RenderTextMethod(vecPos, col, outline, textStart, textCur, font_size, font, false);
                vecPos.x += ImGui::CalcTextSize(font, font_size, textStart, textCur).x;
            }

            const char* colorStart = textCur + 1;
            do
            {
                ++textCur;
            } while (*textCur != '\0' && *textCur != '}');

            ImVec4 textColor;
            if (CGui::GetFontRender()->ProcessInlineHexColor(colorStart, textCur, textColor))
            {
                col = textColor;
            }
            else
            {
                char temp[4096] = { '\0' };
                size_t len = std::min(static_cast<size_t>(textCur - (colorStart - 1)) + (*textCur == '}' ? 1U : 0U), sizeof(temp) - 1);
                strncpy(temp, colorStart - 1, len);
                temp[len] = '\0';
                CGui::GetFontRender()->RenderTextMethod(vecPos, col, outline, temp, temp + len, font_size, font, false);
                vecPos.x += ImGui::CalcTextSize(font, font_size, temp, temp + len).x;
            }

            if (*textCur == '\0')
            {
                textStart = textCur;
                break;
            }

            textStart = textCur + 1;
        }
        else if (*textCur == '\n')
        {
            CGui::GetFontRender()->RenderTextMethod(vecPos, col, outline, textStart, textCur, font_size, font, false);
            vecPos.x = pos.x;
            vecPos.y += font_size;
            textStart = textCur + 1;
        }

        ++textCur;
    }

    if (textCur != textStart)
    {
        CGui::GetFontRender()->RenderTextMethod(vecPos, col, outline, textStart, textCur, font_size, font, false);
        vecPos.x += ImGui::CalcTextSize(font, font_size, textStart, textCur).x;
    }
    else
    {
        vecPos.y += font_size;
    }
}

void CFontRender::Render3DLabel(float fontSize, ImVec2 pos, char* utf8string, uint32_t dwColor)
{
    uint16_t linesCount = 0;
    std::string strString = CUtil::ReplaceKey(std::string(utf8string));
    std::size_t size = strString.length();
    std::string color;

    for (uint32_t i = 0; i < size; i++)
    {
        if (i + 7 < strString.length())
        {
            if (strString[i] == '{' && strString[i + 7] == '}')
            {
                color = strString.substr(i, 7 + 1);
            }
        }

        if (strString[i] == '\n')
        {
            linesCount++;
            if (i + 1 < strString.length() && !color.empty())
            {
                strString.insert(i + 1, color);
                size += color.length();
                color.clear();
            }
        }

        if (strString[i] == '\t')
        {
            strString.replace(i, 1, " ");
        }
    }

    float defaultFontSize = CFontRender::GetFontSize();
    float scaleRatio = fontSize / defaultFontSize;

    float lineHeight = fontSize * 1.2f;
    float totalHeight = lineHeight * (linesCount + 1);

    ImVec2 startPos = pos;
    startPos.y -= totalHeight / 2.0f;
    startPos.y += lineHeight / 2.0f;

    if (linesCount > 0)
    {
        uint16_t curLine = 0;
        uint16_t curIt = 0;
        for (uint32_t i = 0; i <= strString.length(); i++)
        {
            if (i == strString.length() || strString[i] == '\n')
            {
                if (strString[curIt] == '\n')
                {
                    curIt++;
                }

                if (i > curIt)
                {
                    ImVec2 _pos = startPos;

                    float textWidth = CFontRender::CalcTextSizeWithoutTags((char*)strString.substr(curIt, i - curIt).c_str()).x;
                    textWidth *= scaleRatio;
                    _pos.x -= textWidth / 2.0f;

                    _pos.y += curLine * lineHeight;

                    CFontRender::TextWithColorsW(CFontRender::GetFont(), fontSize, 1, _pos, __builtin_bswap32(dwColor), (char*)strString.substr(curIt, i - curIt).c_str());
                }

                curIt = i + 1;
                curLine++;
            }
        }
    }
    else
    {
        float textWidth = CFontRender::CalcTextSizeWithoutTags((char*)strString.c_str()).x;
        textWidth *= scaleRatio;

        pos.x -= textWidth / 2.0f;
        CFontRender::TextWithColorsW(CFontRender::GetFont(), fontSize, 1, pos, __builtin_bswap32(dwColor), (char*)strString.c_str());
    }
}

bool CFontRender::ProcessInlineHexColor(const char* start, const char* end, ImVec4& color)
{
    const int hexCount = (int)(end - start);
    if (hexCount == 6 || hexCount == 8)
    {
        char hex[9];
        strncpy(hex, start, hexCount);
        hex[hexCount] = 0;

        unsigned int hexColor = 0;
        if (sscanf(hex, "%x", &hexColor) > 0)
        {
            color.x = static_cast<float>((hexColor & 0x00FF0000) >> 16) / 255.0f;
            color.y = static_cast<float>((hexColor & 0x0000FF00) >> 8) / 255.0f;
            color.z = static_cast<float>((hexColor & 0x000000FF)) / 255.0f;
            color.w = 1.0f;

            if (hexCount == 8)
            {
                color.w = static_cast<float>((hexColor & 0xFF000000) >> 24) / 255.0f;
            }

            return true;
        }
    }

    return false;
}