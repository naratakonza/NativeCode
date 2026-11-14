#pragma once

#include <cstdint>

#include "Enums/eMaxArchives.h"
#include "Enums/eMaxItem.h"

class CStreaming
{
public:
    static char (&ms_files)[48 * eMaxArchives::MAX_IMG_ARCHIVES];
    static std::uint32_t& ms_memoryUsed;
    static std::uint32_t& ms_memoryAvailable;

    static void Initialise();
    static void InitImageList();
    static int AddImageToList(const char* fileName, bool isPlayerIMG);
    static void RequestModel(unsigned int modelId);
    static void RequestModelComponent(std::uint32_t componentId);
    static void RemoveModel(int modelId, bool fromStreaming);
    static void LoadAllRequestedModels(bool onlyPriorityRequests = false);
    static std::uint8_t IsModelLoaded(unsigned int modelId);
    static void SetModelIsDeletable(int modelId);
    static bool IsObjectInCdImage(int index, const char* opcode);
    static bool WaitForModelToLoad(int modelId);
};