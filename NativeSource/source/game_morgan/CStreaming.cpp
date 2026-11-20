#include "../../Main.h"
#include "CStreaming.h"
#include "CdStreamInfo.h"
#include "../utils/Storage/CStorage.h"

#include <mach-o/dyld.h>

static bool imageListHookReady = false;
static bool stockImageListOpened = false;

char (&CStreaming::ms_files)[48 * eMaxArchives::MAX_IMG_ARCHIVES] = *reinterpret_cast<char (*)[48 * eMaxArchives::MAX_IMG_ARCHIVES]>(SA_ADDR(0xA99400) + _dyld_get_image_vmaddr_slide(0));
std::uint32_t& CStreaming::ms_memoryUsed = *reinterpret_cast<std::uint32_t*>(SA_ADDR(0x8E7E54) + _dyld_get_image_vmaddr_slide(0));
std::uint32_t& CStreaming::ms_memoryAvailable = *reinterpret_cast<std::uint32_t*>(SA_ADDR(0x4E8CCC) + _dyld_get_image_vmaddr_slide(0));

void CStreaming::Initialise()
{
    CNecessary::Log(OBFUSCATE("CStreaming Initialise"));

    imageListHookReady = CdStreamInfo::IsAvailable() && CMemory::HasMappedInstruction(SA_ADDR(0x15551C), "\x04\x07\x0D\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x14D268), "\x73\x4A\x00\x90") && CMemory::HasMappedInstruction(SA_ADDR(0x14D2B0), "\xDF\x82\x00\x71") && CMemory::HasMappedInstruction(SA_ADDR(0x3040C8), "\x1E\x4C\x06\x14");

    if (!imageListHookReady)
    {
        CNecessary::Log(OBFUSCATE("CStreaming InitImageList bridge unavailable"));
    }
}

extern "C" __attribute__((visibility("default"))) void CStreaming_InitImageList_hook()
{
    CStreaming::InitImageList();
}

extern "C" __attribute__((visibility("default"))) int CStreaming_AddImageToList_hook(const char* fileName, bool isPlayerIMG)
{
    const int slot = CStreaming::AddImageToList(fileName, isPlayerIMG);
    return slot < 0 ? 0 : slot;
}

void CStreaming::InitImageList()
{
    if (!pthread_main_np()) {
        return;
    }

    if (!imageListHookReady)
    {
        CStreaming::Initialise();
    }

    if (!imageListHookReady) {
        return;
    }

    std::memset(CStreaming::ms_files, 0, sizeof(CStreaming::ms_files));

    const int gta3 = CStreaming::AddImageToList(OBFUSCATE("TEXDB\\GTA3.IMG"), true);
    const int gtaInt = CStreaming::AddImageToList(OBFUSCATE("TEXDB\\GTA_INT.IMG"), true);
    stockImageListOpened = gta3 == 0 && gtaInt == 1;

    if (stockImageListOpened && morgan::fileio::CStorage::IsAssetFreeBundle())
    {
        CStreaming::AddImageToList(OBFUSCATE("TEXDB\\SAMP.IMG"), true);
        CStreaming::AddImageToList(OBFUSCATE("TEXDB\\SAMPCOL.IMG"), true);
    }
}

int CStreaming::AddImageToList(const char* fileName, bool isPlayerIMG)
{
    if (!imageListHookReady || !pthread_main_np() || !fileName) {
        return -1;
    }

    const auto length = ::strnlen(fileName, 40);
    if (!length || length == 40) {
        return -1;
    }

    const auto asset = morgan::fileio::ResolveGameAssetPath({fileName, length});
    const std::string_view path = asset.resource == morgan::fileio::Resource::ScriptImage ? "SAMP/script.img" : asset.Path();
    if (asset.disposition != morgan::fileio::Disposition::Redirect || path.size() < 5 || path.substr(path.size() - 4) != ".img") {
        return -1;
    }

    for (int i = 0; i < eMaxArchives::MAX_IMG_ARCHIVES; ++i)
    {
        if (CStreaming::ms_files[48 * i] == '\0')
        {
            const int stream = CdStreamInfo::CdStreamOpen(fileName, false);
            if (stream < 0) {
                return -1;
            }

            std::memcpy(&CStreaming::ms_files[48 * i], fileName, length + 1);
            CStreaming::ms_files[48 * i + 40] = isPlayerIMG;
            std::memcpy(&CStreaming::ms_files[48 * i + 44], &stream, sizeof(stream));

            CNecessary::Log(OBFUSCATE("IMG Archive : %s (slot %d, CD handle %u)"), fileName, i, static_cast<std::uint32_t>(stream) >> 24);
            return i;
        }

        const char* existingName = &CStreaming::ms_files[48 * i];
        const auto existingAsset = morgan::fileio::ResolveGameAssetPath({existingName, ::strnlen(existingName, 40)});
        if ((asset.resource == morgan::fileio::Resource::ScriptImage && existingAsset.resource == morgan::fileio::Resource::ScriptImage) || existingAsset.Path() == path)
        {
            return i;
        }
    }

    CNecessary::Log(OBFUSCATE("IMG Out of Space"));
    return -1;
}

void CStreaming::RequestModel(unsigned int modelId)
{
    if (modelId < eMaxItem::MAX_MODELINFO)
    {
        CScripting::ScriptCommand(&request_model, static_cast<int>(modelId));
    }
}

void CStreaming::RequestModelComponent(std::uint32_t componentId)
{
    if (IS_VEHICLE_MOD(componentId))
    {
        CScripting::ScriptCommand(&request_car_component, static_cast<int>(componentId));
    }
}

void CStreaming::LoadAllRequestedModels(bool onlyPriorityRequests)
{
    if (!pthread_main_np() || !stockImageListOpened || !CMemory::HasMappedInstruction(SA_ADDR(0x309B34), "\x6B\x00\x00\x94")) {
        return;
    }

    reinterpret_cast<void (*)(bool)>(SA_ADDR(0x309AE4) + _dyld_get_image_vmaddr_slide(0))(onlyPriorityRequests);
}

void CStreaming::RemoveModel(int modelId, bool fromStreaming)
{
    if (!pthread_main_np() || static_cast<unsigned>(modelId) >= eMaxItem::MAX_MODELINFO || !CStreaming::IsModelLoaded(modelId)) {
        return;
    }

    if (fromStreaming)
    {
        if (CMemory::HasMappedInstruction(SA_ADDR(0x304CDC), "\x15\x20\x29\x9B"))
        {
            reinterpret_cast<void (*)(int)>(SA_ADDR(0x304CC0) + _dyld_get_image_vmaddr_slide(0))(modelId);
        }
    }
    else
    {
        CScripting::ScriptCommand(&release_model, modelId);
    }
}

std::uint8_t CStreaming::IsModelLoaded(unsigned int modelId)
{
    if (modelId >= eMaxItem::MAX_MODELINFO) {
        return false;
    }

    if (IS_VEHICLE_MOD(modelId))
    {
        return CScripting::ScriptCommand(&is_component_available, static_cast<int>(modelId)) != 0;
    }

    return CScripting::ScriptCommand(&is_model_available, static_cast<int>(modelId)) != 0;
}

void CStreaming::SetModelIsDeletable(int modelId)
{
    if (!pthread_main_np() || !stockImageListOpened || static_cast<unsigned>(modelId) >= eMaxItem::MAX_MODELINFO || !CMemory::HasMappedInstruction(SA_ADDR(0x30C3D4), "\x2A\x79\x1E\x12")) {
        return;
    }

    reinterpret_cast<void (*)(int)>(SA_ADDR(0x30C3C0) + _dyld_get_image_vmaddr_slide(0))(modelId);
}

bool CStreaming::IsObjectInCdImage(int index, const char*)
{
    if (static_cast<unsigned>(index) >= eMaxItem::MAX_MODELINFO || !CMemory::HasMappedInstruction(SA_ADDR(0x308210), "\x08\x0D\x40\xB9")) {
        return false;
    }

    return reinterpret_cast<bool (*)(int)>(SA_ADDR(0x308200) + _dyld_get_image_vmaddr_slide(0))(index);
}

bool CStreaming::WaitForModelToLoad(int modelId)
{
    if (!pthread_main_np() || static_cast<unsigned>(modelId) >= eMaxItem::MAX_MODELINFO) {
        return false;
    }

    if (CStreaming::IsModelLoaded(modelId)) {
        return true;
    }

    if (IS_VEHICLE_MOD(modelId))
    {
        CStreaming::RequestModelComponent(modelId);
    }
    else
    {
        CStreaming::RequestModel(modelId);
    }

    CStreaming::LoadAllRequestedModels(false);
    return CStreaming::IsModelLoaded(modelId);
}