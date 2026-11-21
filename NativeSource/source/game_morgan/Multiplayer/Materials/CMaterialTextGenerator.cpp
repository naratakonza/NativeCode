#include "../../../../Main.h"
#include "CMaterialTextGenerator.h"
#include "../../CCommon.h"
#include "../Guis/CGui.h"
#include "../../../vendor/imgui/imgui_impl_renderware.h"
#include <sstream>
#include <strings.h>

CMaterialTextGenerator::CMaterialTextGenerator()
{
    CNecessary::Log(OBFUSCATE("CMaterialTextGenerator Initialise"));

    m_camera = 0;
    m_frame = 0;

    CMaterialTextGenerator::SetUpScene();
}

void CMaterialTextGenerator::SetUpScene()
{
    m_camera = RwCameraCreate();
    m_frame = RwFrameCreate();

    if (!m_camera || !m_frame) {
        return;
    }

    RwObjectHasFrameSetFrame(m_camera, m_frame);

    RwCameraSetFarClipPlane(m_camera, 300.0f);
    RwCameraSetNearClipPlane(m_camera, 0.01f);

    RwV2d const& view = { 0.5f, 0.5f };
    RwCameraSetViewWindow(m_camera, view);
    RwCameraSetProjection(m_camera, rwPERSPECTIVE);

    CCommon::rpWorldAddCamera(m_camera);
}

RwTexture* CMaterialTextGenerator::Create(int iSizeX, int iSizeY, const char* szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char* szText)
{
    RwRaster *raster = RwRasterCreate(iSizeX, iSizeY, 0, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
    if (!raster) {
        return 0;
    }

    if (dwBackgroundColor != 0)
    {
        raster->cFormat = (rwRASTERFORMAT888 >> 8);
    }

    RwTexture *bufferTexture = RwTextureCreate(raster);
    if (!bufferTexture) {
        return 0;
    }

    uint8_t b = (dwBackgroundColor) & 0xFF;
    uint8_t g = (dwBackgroundColor >> 8) & 0xFF;
    uint8_t r = (dwBackgroundColor >> 16) & 0xFF;
    uint8_t a = (dwBackgroundColor >> 24) & 0xFF;

    if (dwBackgroundColor != 0) {
        a = 0xFF;
    }

    uint32_t dwBackground = (r | (g << 8) | (b << 16) | (a << 24));

    RwCamera* previousCamera = CCommon::GetRenderWareCamera();
    m_camera->frameBuffer = raster;
    CCommon::SetRenderWareCamera(m_camera);

    RwCameraClear(m_camera, (RwRGBA*)&dwBackground, 3);
    RwCameraBeginUpdate(m_camera);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODENASHADEMODE);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

    CCommon::DefinedState();

    ImGuiContext* previousContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(CGui::GetFontRender()->GetContext());
    CMaterialTextGenerator::Render(iSizeX, iSizeY, szFontName, byteFontSize, byteFontBold, dwFontColor, byteAlign, szText);

    ImGui::SetCurrentContext(previousContext);
    RwCameraEndUpdate(m_camera);
    CCommon::SetRenderWareCamera(previousCamera);

    if (bufferTexture)
    {
        ++bufferTexture->refCount;
    }

    return bufferTexture;
}

void CMaterialTextGenerator::Render(int iSizeX, int iSizeY, const char* szFontName, uint8_t byteFontSize, [[maybe_unused]] uint8_t byteFontBold, uint32_t dwFontColor, uint8_t byteAlign, const char* szText)
{
    ImVec2 previousSize = ImGui::GetIO().DisplaySize;
    ImGui_ImplRenderWare_NewFrame();
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(iSizeX), static_cast<float>(iSizeY));
    ImGui::NewFrame();
    ImGuiIO &io = ImGui::GetIO();

    char utf8[2048];
    CNecessary::ConvertLocalizedEncodingToUTF8(utf8, szText, 2048);

    // CNecessary::Log("szText %s, Font %s", utf8, szFontName);

    ImFont* fontz;
    if (szFontName && strcasecmp(szFontName, OBFUSCATE("Webdings")) == 0)
    {
        fontz = CGui::GetFontRender()->GetWebdingsFont();
    }
    else if (szFontName && strcasecmp(szFontName, OBFUSCATE("GTAWEAPON3")) == 0)
    {
        fontz = CGui::GetFontRender()->GetWeaponFont();
    }
    else
    {
        fontz = CGui::GetFontRender()->GetFont();
    }

    uint8_t b = (dwFontColor) & 0xFF;
    uint8_t g = (dwFontColor >> 8) & 0xFF;
    uint8_t r = (dwFontColor >> 16) & 0xFF;
    uint8_t a = (dwFontColor >> 24) & 0xFF;

    ImVec2 _veecPos;
    switch (byteAlign)
    {
        case eMaterialTextAlign::MATERIAL_ALIGN_LEFT: {
            ImVec2 veecPos = ImVec2(SCALE_X(io, 0), SCALE_Y(io, 0));

            CGui::GetFontRender()->TextWithColors(fontz, byteFontSize, false, veecPos, ImColor(r, g, b, a), utf8);
            break;
        }

        case eMaterialTextAlign::MATERIAL_ALIGN_CENTER: {
            int newLineCount = 0;
            std::string strText = utf8;
            std::stringstream ssLine(strText);
            std::string tmpLine;

            while (std::getline(ssLine, tmpLine, '\n'))
            {
                if (newLineCount == 0)
                {
                    _veecPos.y = (iSizeY - CGui::GetFontRender()->CalcTextSizeWithoutTags(byteFontSize, tmpLine.c_str()).y) / 2;
                }
                else
                {
                    _veecPos.y -= CGui::GetFontRender()->CalcTextSizeWithoutTags(byteFontSize, tmpLine.c_str()).y / 2;
                }

                newLineCount++;
            }

            if (_veecPos.y < 0.0) {
                _veecPos.y = 0.0;
            }

            std::string strTexts = utf8;
            std::stringstream ssLines(strTexts);
            std::string tmpLines;
            while (std::getline(ssLines, tmpLines, '\n'))
            {
                if (tmpLines[0] != 0)
                {
                    _veecPos.x = (iSizeX - CGui::GetFontRender()->CalcTextSizeWithoutTags(byteFontSize, tmpLines.c_str()).x) / 2;
                    CGui::GetFontRender()->TextWithColors(fontz, byteFontSize, false, _veecPos, ImColor(r, g, b, a), tmpLines.c_str());
                    _veecPos.y += byteFontSize;
                }
            }

            break;
        }

        case eMaterialTextAlign::MATERIAL_ALIGN_RIGHT: {
            std::string strText = utf8;
            std::stringstream ssLine(strText);
            std::string tmpLine;

            while (std::getline(ssLine, tmpLine, '\n'))
            {
                if (tmpLine[0] != 0)
                {
                    _veecPos.x = (iSizeX - CGui::GetFontRender()->CalcTextSizeWithoutTags(byteFontSize, tmpLine.c_str()).x);
                    CGui::GetFontRender()->TextWithColors(fontz, byteFontSize, false, _veecPos, ImColor(r, g, b, a), tmpLine.c_str());
                    _veecPos.y += byteFontSize;
                }
            }

            break;
        }
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplRenderWare_RenderDrawData(ImGui::GetDrawData());
    ImGui::GetIO().DisplaySize = previousSize;
}