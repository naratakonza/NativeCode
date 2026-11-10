#include "../../../Main.h"
#include "imgui.h"
#include "imgui_impl_renderware.h"

#include <mach-o/dyld.h>
#include <vector>

static RwRaster* fontRaster = nullptr;
static std::intptr_t gtaSlide = 0;

bool ImGui_ImplRenderWare_Init()
{
    const bool screen = CMemory::HasMappedInstruction(SA_ADDR(0x3BDD0), "\xE8\x31\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x3BDF4), "\xE8\x31\x00\xB0");
    const bool sprite = CMemory::HasMappedInstruction(SA_ADDR(0x302E1C), "\x20\x05\x46\xBD") && CMemory::HasMappedInstruction(SA_ADDR(0x302E2C), "\x21\x01\x46\xBD");
    if (!screen || !sprite || !RwImageCreate || !RwImageAllocatePixels || !RwImageFindRasterFormat || !RwImageDestroy ||
        !RwRasterCreate || !RwRasterSetFromImage || !RwRasterDestroy || !RwRenderStateGet || !RwRenderStateSet || !RwIm2DRenderIndexedPrimitive) {
        return false;
    }
    gtaSlide = _dyld_get_image_vmaddr_slide(0);
    return true;
}

static bool CreateFontRaster()
{
    CNecessary::Log(OBFUSCATE("GUI | CreateDeviceObjects."));

    auto* atlas = ImGui::GetIO().Fonts;
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    atlas->GetTexDataAsRGBA32(&pixels, &width, &height);
    CNecessary::Log(OBFUSCATE("GUI | Font atlas width: %d, height: %d, depth: %d"), width, height, 32);
    if (!pixels || width <= 0 || height <= 0) {
        return false;
    }

    auto* image = RwImageCreate(width, height, 32);
    if (!image) {
        return false;
    }
    if (!RwImageAllocatePixels(image) || !image->cpPixels || image->stride < width * 4)
    {
        RwImageDestroy(image);
        return false;
    }
    for (int y = 0; y < height; ++y) {
        std::memcpy(image->cpPixels + y * image->stride, pixels + y * width * 4, static_cast<std::size_t>(width) * 4);
    }

    RwInt32 rasterWidth = 0, rasterHeight = 0, depth = 0, flags = 0;
    if (RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, &rasterWidth, &rasterHeight, &depth, &flags))
    {
        fontRaster = RwRasterCreate(rasterWidth, rasterHeight, depth, flags);
        if (fontRaster && !RwRasterSetFromImage(fontRaster, image))
        {
            RwRasterDestroy(fontRaster);
            fontRaster = nullptr;
        }
    }
    RwImageDestroy(image);
    atlas->TexID = fontRaster;
    return fontRaster != nullptr;
}

bool ImGui_ImplRenderWare_NewFrame()
{
    using ScreenDimension = int (*)();
    const int width = reinterpret_cast<ScreenDimension>(SA_ADDR(0x3BDD0) + gtaSlide)();
    const int height = reinterpret_cast<ScreenDimension>(SA_ADDR(0x3BDF4) + gtaSlide)();
    if (width <= 0 || height <= 0) {
        return false;
    }
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 displaySize(static_cast<float>(width), static_cast<float>(height));
    if (io.DisplaySize.x != displaySize.x || io.DisplaySize.y != displaySize.y) {
        CNecessary::Log(OBFUSCATE("GUI | Display size: %f, %f"), displaySize.x, displaySize.y);
    }
    io.DisplaySize = displaySize;
    return fontRaster || CreateFontRaster();
}

void ImGui_ImplRenderWare_RenderDrawData(const ImDrawData* drawData)
{
    if (!drawData || drawData->TotalVtxCount == 0) {
        return;
    }

    const RwRenderState states[] = {
        rwRENDERSTATEZTESTENABLE, rwRENDERSTATEZWRITEENABLE, rwRENDERSTATEVERTEXALPHAENABLE,
        rwRENDERSTATESRCBLEND, rwRENDERSTATEDESTBLEND, rwRENDERSTATEFOGENABLE,
        rwRENDERSTATECULLMODE, rwRENDERSTATEALPHATESTFUNCTION, rwRENDERSTATEALPHATESTFUNCTIONREF,
        rwRENDERSTATETEXTUREFILTER, rwRENDERSTATETEXTUREADDRESS, rwRENDERSTATETEXTURERASTER,
    };
    constexpr std::uintptr_t values[] = {
        0, 0, 1, rwBLENDSRCALPHA, rwBLENDINVSRCALPHA, 0, rwCULLMODECULLNONE,
        rwALPHATESTFUNCTIONGREATER, 2, rwFILTERLINEAR, rwTEXTUREADDRESSCLAMP,
    };
    constexpr std::size_t count = sizeof(states) / sizeof(states[0]);
    static_assert(sizeof(values) / sizeof(values[0]) + 1 == count);
    static_assert(sizeof(ImDrawIdx) == sizeof(RwImVertexIndex));
    static_assert(sizeof(RwIm2DVertex) == 28);
    void* saved[count]{};
    for (std::size_t i = 0; i < count; ++i)
    {
        if (!RwRenderStateGet(states[i], &saved[i])) {
            return;
        }
    }

    const float screenZ = *reinterpret_cast<const float*>(SA_ADDR(0x867604) + gtaSlide);
    const float recipNearClip = *reinterpret_cast<const float*>(SA_ADDR(0x867600) + gtaSlide);
    if (!std::isfinite(screenZ) || !std::isfinite(recipNearClip) || recipNearClip <= 0.0f) {
        return;
    }
    // This title uses the full-screen background list, so no game scissor changes are needed.
    static std::vector<RwIm2DVertex> vertices;
    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex)
    {
        const auto& list = *drawData->CmdLists[listIndex];
        const auto oldCapacity = vertices.capacity();
        vertices.resize(static_cast<std::size_t>(list.VtxBuffer.Size));
        if (vertices.capacity() != oldCapacity) {
            CNecessary::Log(OBFUSCATE("GUI | Vertex buffer reallocated. Size: %zu"), vertices.capacity());
        }
        for (int index = 0; index < list.VtxBuffer.Size; ++index)
        {
            const auto& src = list.VtxBuffer[index];
            auto& dst = vertices[static_cast<std::size_t>(index)];
            dst.x = src.pos.x - drawData->DisplayPos.x;
            dst.y = src.pos.y - drawData->DisplayPos.y;
            dst.z = screenZ;
            dst.rhw = recipNearClip;
            dst.u = src.uv.x;
            dst.v = src.uv.y;
            RwIm2DVertexSetIntRGBA(&dst, (src.col >> IM_COL32_R_SHIFT) & 0xFF, (src.col >> IM_COL32_G_SHIFT) & 0xFF,
                                 (src.col >> IM_COL32_B_SHIFT) & 0xFF, (src.col >> IM_COL32_A_SHIFT) & 0xFF);
        }

        auto* indices = list.IdxBuffer.Data;
        for (const auto& command : list.CmdBuffer)
        {
            if (!command.UserCallback && command.TextureId == fontRaster && command.ElemCount)
            {
                for (std::size_t i = 0; i < count - 1; ++i) {
                    RwRenderStateSet(states[i], reinterpret_cast<void*>(values[i]));
                }
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, fontRaster);
                RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, vertices.data(), list.VtxBuffer.Size, indices, command.ElemCount);
            }
            indices += command.ElemCount;
        }
    }
    for (std::size_t i = count; i > 0; --i) {
        RwRenderStateSet(states[i - 1], saved[i - 1]);
    }
}

void ImGui_ImplRenderWare_ShutDown()
{
    CNecessary::Log(OBFUSCATE("ImGui ShutDown."));

    if (fontRaster)
    {
        RwRasterDestroy(fontRaster);
        fontRaster = nullptr;
    }
    ImGui::GetIO().Fonts->TexID = nullptr;
}
