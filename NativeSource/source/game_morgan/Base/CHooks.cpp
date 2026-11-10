#include "../../../Main.h"
#include "CHooks.h"
#include "../Models/CModelInfo.h"
#include "../CPools.h"
#include "../CStreaming.h"
#include "../RadarMod/CTiltedRadar.h"
#include "../../utils/Memory/CHook.h"
#include "../../utils/Storage/CStorage.h"
#include "../Multiplayer/CMultiPlayer.h"
#include "../Multiplayer/Guis/CGui.h"

static bool gameProcessHookInstalled = false;
static bool renderTitleHookInstalled = false;

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
        CExceptions::Print(OBFUSCATE("CGame Process"), error.what());
        throw;
    }
    catch (...)
    {
        CExceptions::Print(OBFUSCATE("CGame Process"), OBFUSCATE("unknown C++ exception"));
        throw;
    }

    if (pthread_main_np() && gameProcessHookInstalled && CMultiPlayer::IsSampActive())
    {
        CMultiPlayer::Process();
    }
}

void CHooks::InstallNecessary()
{
    if (const auto error = CHook::GetThread()->InstallFileOpen())
    {
        CNecessary::Log(OBFUSCATE("Client ONLOAD failed (error %d)"), error.value());
    }
}

extern "C" __attribute__((visibility("default"))) void RenderTitleEndFrame_hook(void (*EndFrame2D)())
{
    EndFrame2D();

    if (pthread_main_np() && renderTitleHookInstalled && CMultiPlayer::IsSampActive())
    {
        CMultiPlayer::m_bGameStarted = true;

        if (CMultiPlayer::IsSampInit() && CMultiPlayer::GetActiveNetwork() && CGui::GetFontRender())
        {
            CGui::GetFontRender()->Render();
        }
    }
}

void CHooks::InstallMenu()
{
    gameProcessHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x151B30), "\xE9\x23\xBA\x6D") && CMemory::HasMappedInstruction(SA_ADDR(0x15A0C8), "\xCE\xF3\x0C\x94");

    if (!gameProcessHookInstalled)
    {
        CExceptions::Print(OBFUSCATE("CHooks InstallMenu"), OBFUSCATE("CGame Process bridge unavailable"));
    }

    // Both GTA 2D paths reach this call before RwCameraEndUpdate.
    renderTitleHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x319178), "\xC0\x03\x5F\xD6") && CMemory::HasMappedInstruction(SA_ADDR(0x15A2D0), "\xA7\xF3\x0C\x94");
    if (!renderTitleHookInstalled) {
        CExceptions::Print(OBFUSCATE("CHooks InstallMenu"), OBFUSCATE("End-of-frame title bridge unavailable"));
    }
}

void CHooks::SetHooking(eHookType type)
{
    CNecessary::Log(OBFUSCATE("InstallSetHooking type : %d"), static_cast<int>(type));

    switch (type) {
        case eHookType::HOOK_TYPE_ONLOAD: {
            CHooks::InstallNecessary();
            CModelInfo::Initialise();
            CPools::Install();

            break;
        }

        case eHookType::HOOK_TYPE_START: {
            CHooks::InstallMenu();
            CStreaming::Initialise();

            break;
        }

        case eHookType::HOOK_TYPE_GAME: {
            CTiltedRadar::Initialise();

            break;
        }
    }
}
