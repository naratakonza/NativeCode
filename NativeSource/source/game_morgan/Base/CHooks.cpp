#include "../../../Main.h"
#include "CHooks.h"
#include "CCrashFix.h"
#include "CPad.h"
#include "Graphics/CRealTimeShadowManager.h"
#include "CWidgets.h"
#include "MultiTouch/CTouchEvent.h"
#include "../CAspectRatio.h"
#include "../CDraw.h"
#include "../CGyro.h"
#include "../CHud.h"
#include "../CRadar.h"
#include "../CCameraController.h"
#include "../CTimer.h"
#include "../CWorld.h"
#include "../Models/CModelInfo.h"
#include "../CPools.h"
#include "../CPedIntelligence.h"
#include "../CUtil.h"
#include "../Tasks/TaskTypes/CTaskSimplePlayerOnFoot.h"
#include "../Enums/eWeaponSampid.h"
#include "../CStreaming.h"
#include "../CTxdManager.h"
#include "../Entity/Ped/CPlayerPed.h"
#include "../Entity/CPhysical.h"
#include "../CStats.h"
#include "../FrontEnd/CWidgetPlayerInfo.h"
#include "../RadarMod/CTiltedRadar.h"
#include "../../utils/Memory/CHook.h"
#include "../../utils/Storage/CStorage.h"
#include "../Multiplayer/CMultiPlayer.h"
#include "../Multiplayer/Pools/CNetGamePool.h"
#include "../Multiplayer/Audio/CAudio.h"
#include "../Multiplayer/Guis/CGui.h"
#include "../Multiplayer/Guis/CInterface.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <mach-o/dyld.h>
#include <strings.h>
#include <memory>
#include <sys/stat.h>

extern "C"
{
    uintptr_t PickUpPickup_jump_back = SA_ADDR(0xCE310) + _dyld_get_image_vmaddr_slide(0);
}

static bool gameProcessHookInstalled = false;
static bool renderTitleHookInstalled = false;
static bool clothesRebuildHookInstalled = false;
static bool indicatorHooksInstalled = false;
static bool cameraGyroHooksInstalled = false;

uintptr_t pSavedAimTarget = 0;

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

    if (pthread_main_np() && CMultiPlayer::IsSampInit() && CMultiPlayer::GetActiveNetwork())
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool())
        {
            try
            {
                CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->SnapshotProcess();
            }
            catch (const std::exception& error)
            {
                CExceptions::Print(OBFUSCATE("s008"), error.what());
            }
            catch (...)
            {
                CExceptions::Print(OBFUSCATE("s008"), OBFUSCATE("unknown C++ exception"));
            }
        }
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

            CInterface::Control();

            if (CGui::GetScoreBoard()->GetActiveScore() /*|| CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() || (CGui::GetVoiceMenu()->GetActiveVoiceMenu() && !CVoiceMenu::m_bCustomizeMode) || CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/) {
				return;
			}

            if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()) 
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetTextDrawPool()->Draw();
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

extern "C" __attribute__((visibility("default"))) void SendPickedUpPickup_Helper(uintptr_t pickupAddr)
{
	if (pickupAddr)
	{
		uintptr_t aPickUps_addr = SA_ADDR(0x6B8AA0) + _dyld_get_image_vmaddr_slide(0);
		int pickupIndex = (pickupAddr - aPickUps_addr) / 0x24;

		if (CMultiPlayer::IsSampActive() && CMultiPlayer::GetActiveNetwork() && CMultiPlayer::GetActiveNetwork()->GetNetGamePools())
		{
			if (CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPickupPool())
			{
				CMultiPlayer::GetActiveNetwork()->GetNetGamePools()->GetPickupPool()->SendPickedUpPickupByGTAId(pickupIndex);
			}
		}
	}
}

extern "C" __attribute__((naked, visibility("default"))) void PickUpPickup_hook()
{
    __asm__ volatile (
        "sub sp, sp, #256\n"
        "stp x0, x1, [sp, #0]\n"
        "stp x2, x3, [sp, #16]\n"
        "stp x4, x5, [sp, #32]\n"
        "stp x6, x7, [sp, #48]\n"
        "stp x8, x9, [sp, #64]\n"
        "stp x10, x11, [sp, #80]\n"
        "stp x12, x13, [sp, #96]\n"
        "stp x14, x15, [sp, #112]\n"
        "stp x16, x17, [sp, #128]\n"
        "stp x18, x19, [sp, #144]\n"
        "stp x20, x21, [sp, #160]\n"
        "stp x22, x23, [sp, #176]\n"
        "stp x24, x25, [sp, #192]\n"
        "stp x26, x27, [sp, #208]\n"
        "stp x28, x29, [sp, #224]\n"
        "str x30, [sp, #240]\n"
        "sub sp, sp, #528\n"
        "stp q0, q1, [sp, #0]\n"
        "stp q2, q3, [sp, #32]\n"
        "stp q4, q5, [sp, #64]\n"
        "stp q6, q7, [sp, #96]\n"
        "stp q8, q9, [sp, #128]\n"
        "stp q10, q11, [sp, #160]\n"
        "stp q12, q13, [sp, #192]\n"
        "stp q14, q15, [sp, #224]\n"
        "stp q16, q17, [sp, #256]\n"
        "stp q18, q19, [sp, #288]\n"
        "stp q20, q21, [sp, #320]\n"
        "stp q22, q23, [sp, #352]\n"
        "stp q24, q25, [sp, #384]\n"
        "stp q26, q27, [sp, #416]\n"
        "stp q28, q29, [sp, #448]\n"
        "stp q30, q31, [sp, #480]\n"
        "mrs x9, nzcv\n"
        "str x9, [sp, #512]\n"
        "mov x0, x19\n"
        "bl _SendPickedUpPickup_Helper\n"
        "ldr x9, [sp, #512]\n"
        "msr nzcv, x9\n"
        "ldp q0, q1, [sp, #0]\n"
        "ldp q2, q3, [sp, #32]\n"
        "ldp q4, q5, [sp, #64]\n"
        "ldp q6, q7, [sp, #96]\n"
        "ldp q8, q9, [sp, #128]\n"
        "ldp q10, q11, [sp, #160]\n"
        "ldp q12, q13, [sp, #192]\n"
        "ldp q14, q15, [sp, #224]\n"
        "ldp q16, q17, [sp, #256]\n"
        "ldp q18, q19, [sp, #288]\n"
        "ldp q20, q21, [sp, #320]\n"
        "ldp q22, q23, [sp, #352]\n"
        "ldp q24, q25, [sp, #384]\n"
        "ldp q26, q27, [sp, #416]\n"
        "ldp q28, q29, [sp, #448]\n"
        "ldp q30, q31, [sp, #480]\n"
        "add sp, sp, #528\n"
        "ldp x0, x1, [sp, #0]\n"
        "ldp x2, x3, [sp, #16]\n"
        "ldp x4, x5, [sp, #32]\n"
        "ldp x6, x7, [sp, #48]\n"
        "ldp x8, x9, [sp, #64]\n"
        "ldp x10, x11, [sp, #80]\n"
        "ldp x12, x13, [sp, #96]\n"
        "ldp x14, x15, [sp, #112]\n"
        "ldp x16, x17, [sp, #128]\n"
        "ldp x18, x19, [sp, #144]\n"
        "ldp x20, x21, [sp, #160]\n"
        "ldp x22, x23, [sp, #176]\n"
        "ldp x24, x25, [sp, #192]\n"
        "ldp x26, x27, [sp, #208]\n"
        "ldp x28, x29, [sp, #224]\n"
        "ldr x30, [sp, #240]\n"
        "add sp, sp, #256\n"
        "ldrb w8, [x19, #0x20]\n"
        "adrp x16, _PickUpPickup_jump_back@PAGE\n"
        "ldr x16, [x16, _PickUpPickup_jump_back@PAGEOFF]\n"
        "br x16\n"
    );
}

extern "C" __attribute__((visibility("default"))) uintptr_t TextureDatabaseRuntime_GetTexture_hook(const char* szTextureName, uintptr_t (*TextureDatabaseRuntime_GetTexture)(const char*))
{
    if (!szTextureName || szTextureName[0] == '\0') {
        return 0;
    }

    const uintptr_t registered = SA_ADDR(0x801F90) + _dyld_get_image_vmaddr_slide(0);
    auto& iDatabaseCount = *reinterpret_cast<uint32_t*>(registered + 4);
    auto* pDatabaseList = *reinterpret_cast<uintptr_t**>(registered + 8);
    if (!pDatabaseList && iDatabaseCount != 0) {
        return 0;
    }

    if (pDatabaseList && iDatabaseCount != 0)
    {
        auto* end = pDatabaseList + iDatabaseCount;
        auto* firstNull = std::find(pDatabaseList, end, uintptr_t{0});
        if (firstNull != end)
        {
            iDatabaseCount = static_cast<uint32_t>(std::remove(firstNull, end, uintptr_t{0}) - pDatabaseList);
        }
    }

    uintptr_t texture = TextureDatabaseRuntime_GetTexture(szTextureName);
    if (texture) {
        return texture;
    }

    pDatabaseList = *reinterpret_cast<uintptr_t**>(registered + 8);
    if (!pDatabaseList || iDatabaseCount == 0) {
        return 0;
    }

    std::string szTextureNameLower = szTextureName;
    std::transform(szTextureNameLower.begin(), szTextureNameLower.end(), szTextureNameLower.begin(), [](unsigned char c) { return std::tolower(c); });

    for (uint32_t iDatabaseIndex = 0; iDatabaseIndex < iDatabaseCount; ++iDatabaseIndex)
    {
        const uintptr_t textureDatabase = pDatabaseList[iDatabaseIndex];
        if (!textureDatabase) {
            continue;
        }

        const char* szDatabaseName = *reinterpret_cast<const char* const*>(textureDatabase + 8);
        std::string szTargetName = szTextureNameLower;
        if (szDatabaseName && szDatabaseName[0] != '\0')
        {
            CTxdManager::ParseDatabaseRedirects(szDatabaseName);
            auto dbIt = CTxdManager::m_mapDatabaseRedirects.find(szDatabaseName);
            if (dbIt != CTxdManager::m_mapDatabaseRedirects.end())
            {
                auto redirIt = dbIt->second.find(szTextureNameLower);
                if (redirIt != dbIt->second.end())
                {
                    szTargetName = redirIt->second;
                }
            }
        }

        if (szTargetName != szTextureNameLower)
        {
            const uintptr_t redirectedTexture = TextureDatabaseRuntime_GetTexture(szTargetName.c_str());
            if (redirectedTexture) {
                return redirectedTexture;
            }
        }

        const auto* pBucketTable = *reinterpret_cast<const uint32_t* const*>(textureDatabase + 0x108);
        const uint32_t bucketTableSize = *reinterpret_cast<const uint32_t*>(textureDatabase + 0x110);
        const auto* pEntryList = *reinterpret_cast<const uint8_t* const*>(textureDatabase + 0x28);
        if (!pBucketTable || !pEntryList || bucketTableSize == 0 || !TextureDatabaseRuntime_GetRWTexture) {
            continue;
        }

        const uint32_t iEntryCount = pBucketTable[bucketTableSize];
        const uint32_t databaseEntryCount = *reinterpret_cast<const uint32_t*>(textureDatabase + 0x24);
        if (iEntryCount == 0 || iEntryCount > 100000 || iEntryCount > databaseEntryCount) {
            continue;
        }

        for (uint32_t iEntryIndex = 0; iEntryIndex < iEntryCount; ++iEntryIndex)
        {
            const auto* pEntry = pEntryList + iEntryIndex * 31;
            const char* szCurrentName = nullptr;
            std::memcpy(&szCurrentName, pEntry, sizeof(szCurrentName));
            bool matched = szCurrentName && strcasecmp(szCurrentName, szTargetName.c_str()) == 0;

            if ((pEntry[0xE] & 4) != 0)
            {
                const char* affiliateName = nullptr;
                std::memcpy(&affiliateName, pEntry + 23, sizeof(affiliateName));
                if (affiliateName && (matched || strcasecmp(affiliateName, szTargetName.c_str()) == 0))
                {
                    const uintptr_t affiliateTexture = TextureDatabaseRuntime_GetTexture(affiliateName);
                    if (affiliateTexture) {
                        return affiliateTexture;
                    }
                }

                continue;
            }

            if (matched)
            {
                auto* matchedTexture = TextureDatabaseRuntime_GetRWTexture(reinterpret_cast<void*>(textureDatabase), static_cast<int>(iEntryIndex));
                if (matchedTexture) {
                    return reinterpret_cast<uintptr_t>(matchedTexture);
                }
            }
        }
    }

    return 0;
}

bool (*CPedIntelligence_TestForStealthKill)(CPedIntelligence* thiz, CPedGta* pTarget, bool bUnknown);
extern "C" __attribute__((visibility("default"))) bool CPedIntelligence_TestForStealthKill_hook(CPedIntelligence* thiz, CPedGta* pTarget, bool bUnknown, bool (*original)(CPedIntelligence*, CPedGta*, bool))
{
    if (!indicatorHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive()) {
        return original(thiz, pTarget, bUnknown);
    }

    CPedIntelligence_TestForStealthKill = original;

    CPedGta* pPlayerPed = CUtil::GamePool_FindPlayerPed();
    if (pPlayerPed && thiz && thiz->m_pPed == pPlayerPed)
    {
        bool isKnife = false;
        int8_t slot = pPlayerPed->byteCurWeaponSlot;
        if (slot >= 0 && slot < 13)
        {
            uint32_t weaponType = pPlayerPed->WeaponSlots[slot].dwType;
            if (weaponType == eWeaponSampId::WEAPON_KNIFE)
            {
                isKnife = true;
            }
        }

        if (isKnife && pTarget && CUtil::IsPedPointerValid(reinterpret_cast<uintptr_t>(pTarget)))
        {
            if (pTarget->fHealth > 0.1f)
            {
                CVector playerPos = (pPlayerPed->mat != nullptr) ? pPlayerPed->mat->pos : pPlayerPed->m_transform.m_vPosn;
                CVector targetPos = (pTarget->mat != nullptr) ? pTarget->mat->pos : pTarget->m_transform.m_vPosn;

                float dist = CUtil::GetDistance(playerPos, targetPos);
                if (dist <= 2.5f)
                {
                    CVector targetForward;
                    if (pTarget->mat != nullptr)
                    {
                        targetForward = pTarget->mat->up;
                    }
                    else
                    {
                        float heading = pTarget->m_transform.m_fHeading;
                        targetForward.x = -std::sin(heading);
                        targetForward.y = std::cos(heading);
                        targetForward.z = 0.0f;
                    }

                    CVector dirToPlayer;
                    dirToPlayer.x = playerPos.x - targetPos.x;
                    dirToPlayer.y = playerPos.y - targetPos.y;
                    dirToPlayer.z = 0.0f;

                    float dirLen = std::sqrt(dirToPlayer.x * dirToPlayer.x + dirToPlayer.y * dirToPlayer.y);
                    if (dirLen > 0.0f)
                    {
                        dirToPlayer.x /= dirLen;
                        dirToPlayer.y /= dirLen;
                    }

                    float dot = dirToPlayer.x * targetForward.x + dirToPlayer.y * targetForward.y;
                    if (dot < -0.5f) // อยู่ด้านหลังเท่านั้น
                    {
                        return true;
                    }
                }
            }
        }
    }

    return CPedIntelligence_TestForStealthKill(thiz, pTarget, bUnknown);
}

void (*CPlayerPed_DrawTriangleForMouseRecruitPed)(uintptr_t thiz);
extern "C" __attribute__((visibility("default"))) void CPlayerPed_DrawTriangleForMouseRecruitPed_hook(uintptr_t thiz, void (*original)(uintptr_t))
{
    if (!indicatorHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive()) {
        original(thiz);
        return;
    }

    CPlayerPed_DrawTriangleForMouseRecruitPed = original;

    if (thiz == 0) {
        return;
    }

    CPedGta* pPlayerPed = reinterpret_cast<CPedGta*>(thiz);

    bool isBlockPunch = false;
    bool isMelee = false;
    bool isKnife = false;
    int8_t slot = pPlayerPed->byteCurWeaponSlot;
    if (slot >= 0 && slot < 13)
    {
        uint32_t weaponType = pPlayerPed->WeaponSlots[slot].dwType;
        if (CPad::m_iSaveBlockPunch)
        {
            isBlockPunch = true;
        }
        else if (weaponType > 0 && weaponType <= eWeaponSampId::WEAPON_CANE)
        {
            isMelee = true;
            if (weaponType == eWeaponSampId::WEAPON_KNIFE)
            {
                isKnife = true;
            }
        }
    }

    static bool m_bWasMeleeAiming = false;
    if (isMelee || isBlockPunch)
    {
        *reinterpret_cast<uintptr_t*>(thiz + 0x988) = 0;
        pSavedAimTarget = CUtil::FindFistTarget(pPlayerPed);
        m_bWasMeleeAiming = true;
    }
    else
    {
        pSavedAimTarget = 0;
        if (m_bWasMeleeAiming)
        {
            *reinterpret_cast<uintptr_t*>(thiz + 0x988) = 0;
            m_bWasMeleeAiming = false;
        }

        return CPlayerPed_DrawTriangleForMouseRecruitPed(thiz);
    }

    if (pSavedAimTarget != 0 && CUtil::IsPedPointerValid(pSavedAimTarget))
    {
        uint8_t flags = *reinterpret_cast<uint8_t*>(pSavedAimTarget + 0x5A);

        if ((flags & 7) == 3)
        {
            *reinterpret_cast<uintptr_t*>(thiz + 0x988) = pSavedAimTarget;
        }
    }

    if (isKnife)
    {
        bool isBehind = false;
        if (pSavedAimTarget != 0 && CUtil::IsPedPointerValid(pSavedAimTarget))
        {
            CPedGta* pTargetPed = reinterpret_cast<CPedGta*>(pSavedAimTarget);
            if (pTargetPed->fHealth > 0.1f)
            {
                CVector playerPos = (pPlayerPed->mat != nullptr) ? pPlayerPed->mat->pos : pPlayerPed->m_transform.m_vPosn;
                CVector targetPos = (pTargetPed->mat != nullptr) ? pTargetPed->mat->pos : pTargetPed->m_transform.m_vPosn;

                float dist = CUtil::GetDistance(playerPos, targetPos);
                if (dist <= 2.5f) // กำหนดระยะว่าจะให้ทำงานไกล้ขนาดไหน
                {
                    CVector targetForward;
                    if (pTargetPed->mat != nullptr)
                    {
                        targetForward = pTargetPed->mat->up;
                    }
                    else
                    {
                        float heading = pTargetPed->m_transform.m_fHeading;
                        targetForward.x = -std::sin(heading);
                        targetForward.y = std::cos(heading);
                        targetForward.z = 0.0f;
                    }

                    CVector dirToPlayer;
                    dirToPlayer.x = playerPos.x - targetPos.x;
                    dirToPlayer.y = playerPos.y - targetPos.y;
                    dirToPlayer.z = 0.0f;

                    float dirLen = std::sqrt(dirToPlayer.x * dirToPlayer.x + dirToPlayer.y * dirToPlayer.y);
                    if (dirLen > 0.0f)
                    {
                        dirToPlayer.x /= dirLen;
                        dirToPlayer.y /= dirLen;
                    }

                    float dot = dirToPlayer.x * targetForward.x + dirToPlayer.y * targetForward.y;
                    if (dot < -0.5f) // อยู่ด้านหลังเท่านั้น
                    {
                        isBehind = true;
                    }
                }
            }
        }

        if (isBehind)
        {
            pPlayerPed->m_pEntLockOnTarget = reinterpret_cast<CEntityGta*>(pSavedAimTarget);
            *reinterpret_cast<uintptr_t*>(thiz + 0x988) = pSavedAimTarget;
        }
        else
        {
            pPlayerPed->m_pEntLockOnTarget = nullptr;
            *reinterpret_cast<uintptr_t*>(thiz + 0x988) = 0;
        }
    }

    return CPlayerPed_DrawTriangleForMouseRecruitPed(thiz);
}

void (*Process_M16_1stPerson)(uintptr_t thiz, uintptr_t target, float f1, float f2, float f3);
extern "C" __attribute__((visibility("default"))) void Process_M16_1stPerson_hook(uintptr_t thiz, uintptr_t target, float f1, float f2, float f3, void (*original)(uintptr_t, uintptr_t, float, float, float))
{
    if (!cameraGyroHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        original(thiz, target, f1, f2, f3);
        return;
    }

    Process_M16_1stPerson = original;

    Process_M16_1stPerson(thiz, target, f1, f2, f3);

    if (CGyro::IsGyro() && (CGyro::GetGyroMode() == eGyroMode::GYRO_AIM_MODE || CGyro::GetGyroMode() == eGyroMode::GYRO_ALL_MODE))
    {
        float aimSens = CGyro::GetAimSensitivity() / 100.0f;
        float gyroX = CGyro::GetGyroX();
        float gyroY = CGyro::GetGyroY();

        if (fabs(gyroX) < 0.015f)
        {
            gyroX = 0.0f;
        }

        if (fabs(gyroY) < 0.015f)
        {
            gyroY = 0.0f;
        }

        double elapsedTime = CGyro::GetElapsedTime();
        if (elapsedTime <= 0.0)
        {
            elapsedTime = 0.016;
        }

        // เอา fov มาคำนวนความไวด้วย เพื่อความสมดุล
        float fovFactor = CDraw::GetFov() / 70.0f;
        if (fovFactor > 1.0f)
        {
            fovFactor = 1.0f;
        }

        // การหัน (แนวนอน)
        float gyroOffsetY = gyroY * 1.0f * static_cast<float>(elapsedTime) * aimSens * fovFactor;
        // ระยะห่าง (แนวตั้ง)
        float gyroOffsetX = gyroX * 1.0f * static_cast<float>(elapsedTime) * aimSens * fovFactor;

        float* pAlpha = reinterpret_cast<float*>(thiz + 0x94);
        float* pBeta = reinterpret_cast<float*>(thiz + 0x84);

        *pAlpha += gyroOffsetY;
        *pBeta += gyroOffsetX;

        if (*pAlpha > 3.1415926535f)
        {
            *pAlpha -= 2.0f * 3.1415926535f;
        }
        else if (*pAlpha < -3.1415926535f)
        {
            *pAlpha += 2.0f * 3.1415926535f;
        }

        if (*pBeta > 1.047197551f)
        {
            *pBeta = 1.047197551f;
        }
        else if (*pBeta < -1.560796327f)
        {
            *pBeta = -1.560796327f;
        }
    }
}

void (*Process_Rocket)(uintptr_t thiz, uintptr_t target, float f1, float f2, float f3, bool b1);
extern "C" __attribute__((visibility("default"))) void Process_Rocket_hook(uintptr_t thiz, uintptr_t target, float f1, float f2, float f3, bool b1, void (*original)(uintptr_t, uintptr_t, float, float, float, bool))
{
    if (!cameraGyroHooksInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        original(thiz, target, f1, f2, f3, b1);
        return;
    }

    Process_Rocket = original;

    Process_Rocket(thiz, target, f1, f2, f3, b1);

    if (CGyro::IsGyro() && (CGyro::GetGyroMode() == eGyroMode::GYRO_AIM_MODE || CGyro::GetGyroMode() == eGyroMode::GYRO_ALL_MODE))
    {
        float aimSens = CGyro::GetAimSensitivity() / 100.0f;
        float gyroX = CGyro::GetGyroX();
        float gyroY = CGyro::GetGyroY();

        if (fabs(gyroX) < 0.015f)
        {
            gyroX = 0.0f;
        }

        if (fabs(gyroY) < 0.015f)
        {
            gyroY = 0.0f;
        }

        double elapsedTime = CGyro::GetElapsedTime();
        if (elapsedTime <= 0.0)
        {
            elapsedTime = 0.016;
        }

        // เอา fov มาคำนวนความไวด้วย เพื่อความสมดุล
        float fovFactor = CDraw::GetFov() / 70.0f;
        if (fovFactor > 1.0f)
        {
            fovFactor = 1.0f;
        }

        // การหัน (แนวนอน)
        float gyroOffsetY = gyroY * 1.0f * static_cast<float>(elapsedTime) * aimSens * fovFactor;
        // ระยะห่าง (แนวตั้ง)
        float gyroOffsetX = gyroX * 1.0f * static_cast<float>(elapsedTime) * aimSens * fovFactor;

        float* pAlpha = reinterpret_cast<float*>(thiz + 0x94);
        float* pBeta = reinterpret_cast<float*>(thiz + 0x84);

        *pAlpha += gyroOffsetY;
        *pBeta += gyroOffsetX;

        if (*pAlpha > 3.1415926535f)
        {
            *pAlpha -= 2.0f * 3.1415926535f;
        }
        else if (*pAlpha < -3.1415926535f)
        {
            *pAlpha += 2.0f * 3.1415926535f;
        }

        if (*pBeta > 1.047197551f)
        {
            *pBeta = 1.047197551f;
        }
        else if (*pBeta < -1.560796327f)
        {
            *pBeta = -1.560796327f;
        }
    }
}

void CHooks::InstallNecessary()
{
    CMemory::WriteProcessMemory(SA_ADDR(0xCE30C), (uintptr_t)"\xEA\x25\x0F\x14", 4); // กระโดดไป Pickup Update

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

    // ระบบ Indicator: iOS
    indicatorHooksInstalled = CMemory::HasMappedBytes(SA_ADDR(0x273768), "\xFF\x83\x02\xD1\xEF\x3B\x03\x6D", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x26AEB8), "\xFF\x03\x01\xD1\xF6\x57\x01\xA9", 8, "__text") && CMemory::HasMappedInstruction(SA_ADDR(0x159B3C), "\xE2\xF7\x0C\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x3BEF80), "\xD6\x62\x03\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x3BFB10), "\xF2\x5F\x03\x94") && CMemory::HasMappedBytes(SA_ADDR(0x497AC4), "\xE1\xEE\xFF\x90\x21\xA0\x1D\x91\xF0\x00\x00\xD0\x10\xFE\x42\xF9\x00\x02\x1F\xD6\x83\xEE\xFF\xF0\x63\xE0\x3A\x91\xF0\x00\x00\xD0\x10\x02\x43\xF9\x00\x02\x1F\xD6", 40, "__mg_gameproc");

    // Gyro ของกล้องเล็ง M16/สไนเปอร์ และจรวด ทั้งสองสถานะของ iOS
    cameraGyroHooksInstalled = CMemory::HasMappedBytes(SA_ADDR(0x11FEB8), "\xFF\x43\x03\xD1\xED\x33\x06\x6D", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x121584), "\xFF\xC3\x02\xD1\xE9\x23\x06\x6D", 8, "__text") && CMemory::HasMappedInstruction(SA_ADDR(0x11C058), "\xA5\xEE\x0D\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x11C0E4), "\x87\xEE\x0D\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x11C1B0), "\x54\xEE\x0D\x94") && CMemory::HasMappedBytes(SA_ADDR(0x497AEC), "\x42\xE4\xFF\x90\x42\xE0\x3A\x91\xF0\x00\x00\xD0\x10\x06\x43\xF9\x00\x02\x1F\xD6\x43\xE4\xFF\xD0\x63\x10\x16\x91\xF0\x00\x00\xD0\x10\x0A\x43\xF9\x00\x02\x1F\xD6", 40, "__mg_gameproc");
}

void CHooks::SetHooking(eHookType type)
{
    CNecessary::Log(OBFUSCATE("InstallSetHooking type : %d"), static_cast<int>(type));

    switch (type) {
        case eHookType::HOOK_TYPE_ONLOAD: {
            CTimer::Initialise();
            CHooks::InstallNecessary();
            CCrashFix::Initialise();
            CWorld::Initialise();
            CModelInfo::Initialise();
            CPools::Install();
            CRealTimeShadowManager::Initialise();

            break;
        }

        case eHookType::HOOK_TYPE_START: {
            CHooks::InstallMenu();
            CWidgetPlayerInfo::Initialise();
            CStreaming::Initialise();
            CTouchEvent::Initialise();
            CWidgets::Initialise();
            CPad::Initialise();
            CTaskSimplePlayerOnFoot::Initialize();
            CAspectRatio::Initialise();
            CDraw::Initialise();
            CHud::Initialise();
            CPhysical::Initialise();
            CStats::Initialise();
            CRadar::Initialise();

            break;
        }

        case eHookType::HOOK_TYPE_GAME: {
            CTiltedRadar::Initialise();

            break;
        }
    }
}