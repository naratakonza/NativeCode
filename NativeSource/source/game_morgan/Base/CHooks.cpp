#include "../../../Main.h"
#include "CHooks.h"
#include "CWidgets.h"
#include "MultiTouch/CTouchEvent.h"
#include "../CAspectRatio.h"
#include "../CDraw.h"
#include "../CHud.h"
#include "../CCameraController.h"
#include "../CTimer.h"
#include "../CWorld.h"
#include "../Models/CModelInfo.h"
#include "../CPools.h"
#include "../CStreaming.h"
#include "../Entity/Ped/CPlayerPed.h"
#include "../Entity/CPhysical.h"
#include "../CStats.h"
#include "../FrontEnd/CWidgetPlayerInfo.h"
#include "../RadarMod/CTiltedRadar.h"
#include "../../utils/Memory/CHook.h"
#include "../../utils/Storage/CStorage.h"
#include "../Multiplayer/CMultiPlayer.h"
#include "../Multiplayer/Audio/CAudio.h"
#include "../Multiplayer/Guis/CGui.h"

#include <mach-o/dyld.h>
#include <memory>
#include <sys/stat.h>

static bool gameProcessHookInstalled = false;
static bool renderTitleHookInstalled = false;
static bool clothesRebuildHookInstalled = false;

extern "C" __attribute__((visibility("default"))) void InitialiseRenderWare_hook(void* (*load)(const char*, bool, int))
{
    constexpr int DF_DXT = 1;
    constexpr int DF_Default = 6;
    const bool externalOnly = morgan::fileio::CStorage::IsAssetFreeBundle();
    const auto hasTexture = [](const char* path)
    {
        const auto asset = morgan::fileio::ResolveGameAssetPath(path);
        return static_cast<bool>(morgan::fileio::CStorage::OpenActiveResource(nullptr, asset).file);
    };

    CNecessary::Log(OBFUSCATE("Initialise RenderWare"));

    if (externalOnly && hasTexture("texdb/samp/samp.txt") && hasTexture("texdb/samp/samp.dxt.dat") && hasTexture("texdb/samp/samp.dxt.toc") && hasTexture("texdb/samp/samp.dxt.tmb"))
    {
        load(OBFUSCATE("samp"), false, DF_DXT);
        CNecessary::Log(OBFUSCATE("Initialise RenderWare: requested samp DXT"));
    }
    else if (externalOnly)
    {
        CNecessary::Log(OBFUSCATE("Initialise RenderWare: samp DXT files unavailable"));
    }

    const int format = externalOnly ? DF_DXT : DF_Default;
    load(OBFUSCATE("mobile"), false, format);
    load(OBFUSCATE("txd"), false, format);
    load(OBFUSCATE("gta3"), false, format);
    load(OBFUSCATE("gta_int"), false, format);
    if (!externalOnly || hasTexture("texdb/cutscene/cutscene.txt"))
    {
        load(OBFUSCATE("cutscene"), false, DF_Default);
    }

    CGui::InitialiseFont();
}

static bool OS_FileRead_hook(void* file, void* buffer, std::size_t numBytes)
{
    if (!buffer || !numBytes || numBytes > INT32_MAX) {
        return false;
    }

    static const auto OS_FileRead = reinterpret_cast<int (*)(void*, void*, int)>(SA_ADDR(0x034A00) + _dyld_get_image_vmaddr_slide(0));
    return OS_FileRead(file, buffer, static_cast<int>(numBytes)) == 0;
}

static bool RLEDecompress_hook(std::uint8_t* dest, std::size_t destSize, const std::uint8_t* src, std::size_t srcSize, std::size_t segSize, std::uint32_t escape)
{
    if (!segSize || segSize > 32) {
        return false;
    }

    while (destSize && srcSize)
    {
        std::size_t count = 1;
        if (*src == escape)
        {
            if (srcSize < 2) {
                return false;
            }

            count = src[1];
            src += 2;
            srcSize -= 2;
        }

        if (srcSize < segSize || count * segSize > destSize) {
            return false;
        }

        for (std::size_t index = 0; index < count; ++index)
        {
            std::memcpy(dest, src, segSize);
            dest += segSize;
            destSize -= segSize;
        }

        src += segSize;
        srcSize -= segSize;
    }

    return true;
}

extern "C" __attribute__((visibility("default"))) bool LoadFullTexture_hook(void* file, TextureLoadBuffer* texture)
{
    if (!file || !texture || static_cast<const std::uint8_t*>(file)[0x08]) {
        return false;
    }

    std::FILE* stream = nullptr;
    std::memcpy(&stream, file, sizeof(stream));
    if (!stream) {
        return false;
    }

    std::uint32_t storedSize = 0;
    std::uint32_t escape = 0;
    if (!OS_FileRead_hook(file, texture->metadata, sizeof(texture->metadata)) || !OS_FileRead_hook(file, &storedSize, sizeof(storedSize)) || !OS_FileRead_hook(file, &escape, sizeof(escape))) {
        return false;
    }

    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::memcpy(&width, texture->metadata + 4, sizeof(width));
    std::memcpy(&height, texture->metadata + 6, sizeof(height));
    height &= 0x7FFF;

    if (!width || !height || static_cast<std::uint64_t>(width) * height > INT32_MAX / 8) {
        return false;
    }

    static const auto GetTextureSize = reinterpret_cast<int (*)(const void*)>(SA_ADDR(0x2BC054) + _dyld_get_image_vmaddr_slide(0));
    static const auto GetSegmentSize = reinterpret_cast<unsigned int (*)(const void*)>(SA_ADDR(0x2BC2FC) + _dyld_get_image_vmaddr_slide(0));
    const int destSize = GetTextureSize(texture);
    if (destSize <= 0 || (escape && storedSize <= sizeof(escape))) {
        return false;
    }

    const std::size_t readSize = escape ? storedSize - sizeof(escape) : static_cast<std::size_t>(destSize);
    const auto position = std::ftell(stream);
    struct stat info{};
    if (readSize > INT32_MAX || position < 0 || ::fstat(::fileno(stream), &info) != 0 || info.st_size < position) {
        return false;
    }

    std::int32_t fileLength = 0;
    std::int32_t fileStart = 0;
    const auto* fileData = static_cast<const std::uint8_t*>(file);
    std::memcpy(&fileLength, fileData + 0x28, sizeof(fileLength));
    std::memcpy(&fileStart, fileData + 0x2C, sizeof(fileStart));
    auto end = static_cast<std::int64_t>(info.st_size);
    if (fileLength >= 0)
    {
        if (fileStart < 0 || position < fileStart) {
            return false;
        }

        end = std::min(end, static_cast<std::int64_t>(fileStart) + fileLength);
    }
    
    if (end < position || readSize > static_cast<std::uint64_t>(end - position)) {
        return false;
    }

    const auto segSize = escape ? GetSegmentSize(texture) : 1u;
    if (!segSize) {
        return false;
    }

    const auto alignedSize = (static_cast<std::size_t>(destSize) + segSize - 1) / segSize * segSize;
    const auto allocSize = (alignedSize + 7) & ~std::size_t{7};
    using TextureBytes = std::unique_ptr<std::uint8_t, decltype(&std::free)>;
    TextureBytes pixels(static_cast<std::uint8_t*>(std::calloc(1, allocSize)), &std::free);
    if (!pixels) {
        return false;
    }

    if (escape)
    {
        TextureBytes compressed(static_cast<std::uint8_t*>(std::malloc(readSize)), &std::free);
        if (!compressed || !OS_FileRead_hook(file, compressed.get(), readSize) || !RLEDecompress_hook(pixels.get(), alignedSize, compressed.get(), readSize, segSize, escape)) {
            return false;
        }
    }
    else if (!OS_FileRead_hook(file, pixels.get(), readSize))
    {
        return false;
    }

    texture->size = destSize;
    texture->ownsPixels = 1;
    texture->pixels = pixels.release();
    return true;
}

extern "C" __attribute__((visibility("default"))) int DrawMoney_Sprintf_hook(char* buffer, int amount)
{
    constexpr std::size_t capacity = sizeof("-$2147483648");
    const auto magnitude = static_cast<unsigned int>(amount);

    if (amount < 0)
    {
        return std::snprintf(buffer, capacity, static_cast<const char*>(OBFUSCATE("-$%07u")), 0u - magnitude);
    }

    return std::snprintf(buffer, capacity, static_cast<const char*>(OBFUSCATE("$%08u")), magnitude);
}

extern "C" __attribute__((visibility("default"))) void CGame_Process_hook(void (*CGame_Process)())
{
    try
    {
        CGame_Process();
    }
    catch (const std::exception& error)
    {
        CExceptions::Print(OBFUSCATE("s007"), error.what());
        throw;
    }
    catch (...)
    {
        CExceptions::Print(OBFUSCATE("s007"), OBFUSCATE("unknown C++ exception"));
        throw;
    }

    if (pthread_main_np() && gameProcessHookInstalled && CMultiPlayer::IsSampActive())
    {
        CCameraController::AimForTheCamera();
        CMultiPlayer::Process();
    }
}

extern "C" __attribute__((visibility("default"))) void Render2dStuff_hook(void (*EndFrame2D)())
{
    const bool renderSamp = pthread_main_np() && renderTitleHookInstalled;
    if (renderSamp)
    {
        CMultiPlayer::m_bGameStarted = true;
    }

    EndFrame2D();

    if (!renderSamp) {
        return;
    }

    if (CMultiPlayer::GetActiveNetwork())
    {
        if (CMultiPlayer::IsSampInit() && CMultiPlayer::IsGameActive())
        {
            if (CGui::GetChat())
            {
                CGui::GetChat()->Render();
            }

            if (CGui::GetFontRender())
            {
                CGui::GetFontRender()->Render();
            }
        }
    }
}

extern "C" __attribute__((visibility("default"))) void CClothes_RebuildPlayer_hook(uintptr_t thiz, bool bForReplay, void (*CClothes_RebuildPlayer)(uintptr_t, bool))
{
    if (!pthread_main_np() || !clothesRebuildHookInstalled || !CMultiPlayer::IsSampActive())
    {
        CClothes_RebuildPlayer(thiz, bForReplay);
        return;
    }

    if (!CPlayerPed::m_bRebuildPlayer) {
        return;
    }

    CClothes_RebuildPlayer(thiz, bForReplay);
}

extern "C" __attribute__((visibility("default"))) void CAmbientAuidoTrack_Process_hook(uintptr_t thiz, uintptr_t pSound, int PlayTime, void (*Process)(uintptr_t, uintptr_t, int))
{
    if (!CAudio::IsOutdoorAmbienceTrackDisabled())
    {
        Process(thiz, pSound, PlayTime);
        return;
    }

    auto* currArea = reinterpret_cast<std::int32_t*>(SA_ADDR(0x7345F8) + _dyld_get_image_vmaddr_slide(0));
    const std::int32_t previousArea = *currArea;
    *currArea = 1;
    Process(thiz, pSound, PlayTime);
    *currArea = previousArea;
}

void CHooks::InstallNecessary()
{
    if (const auto error = CHook::GetThread()->InstallFileOpen())
    {
        CNecessary::Log(OBFUSCATE("Client ONLOAD failed (error %d)"), error.value());
    }
}

void CHooks::InstallMenu()
{
    gameProcessHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x151B30), "\xE9\x23\xBA\x6D") && CMemory::HasMappedInstruction(SA_ADDR(0x15A0C8), "\xCE\xF3\x0C\x94");

    if (!gameProcessHookInstalled)
    {
        CExceptions::Print(OBFUSCATE("CHooks InstallMenu"), OBFUSCATE("CGame Process bridge unavailable"));
    }

    renderTitleHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x319178), "\xC0\x03\x5F\xD6") && CMemory::HasMappedInstruction(SA_ADDR(0x15A2D0), "\xA7\xF3\x0C\x94");
    if (!renderTitleHookInstalled)
    {
        CExceptions::Print(OBFUSCATE("CHooks InstallMenu"), OBFUSCATE("End-of-frame title bridge unavailable"));
    }

    clothesRebuildHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x86450), "\xF6\x57\xBD\xA9") && CMemory::HasMappedBytes(SA_ADDR(0x49728C), "\x62\xDF\xFF\xF0\x42\x40\x11\x91\xF0\x00\x00\xD0\x10\x5A\x42\xF9\x00\x02\x1F\xD6", 20, "__mg_gameproc");
}

void CHooks::SetHooking(eHookType type)
{
    CNecessary::Log(OBFUSCATE("InstallSetHooking type : %d"), static_cast<int>(type));

    switch (type) {
        case eHookType::HOOK_TYPE_ONLOAD: {
            CTimer::Initialise();
            CHooks::InstallNecessary();
            CWorld::Initialise();
            CModelInfo::Initialise();
            CPools::Install();

            break;
        }

        case eHookType::HOOK_TYPE_START: {
            CHooks::InstallMenu();
            CWidgetPlayerInfo::Initialise();
            CStreaming::Initialise();
            CTouchEvent::Initialise();
            CWidgets::Initialise();
            CAspectRatio::Initialise();
            CDraw::Initialise();
            CHud::Initialise();
            CPhysical::Initialise();
            CStats::Initialise();

            break;
        }

        case eHookType::HOOK_TYPE_GAME: {
            CTiltedRadar::Initialise();

            break;
        }
    }
}