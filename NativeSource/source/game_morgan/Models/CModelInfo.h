#pragma once

#include "CAtomicModelInfo.h"
#include "CPedModelInfo.h"
#include "CVehicleModelInfo.h"
#include "../Enums/eMaxArchives.h"

class CModelInfo
{
public:
    static constexpr int NUM_MODEL_INFOS = eMaxArchives::MAX_MODELINFO_ARCHIVES;
    static constexpr int NUM_ATOMIC_MODEL_INFOS = eMaxArchives::MAX_ATOMIC_ARCHIVES;
    static constexpr int NUM_PED_MODEL_INFOS = eMaxArchives::MAX_SKIN_ARCHIVES;
    static constexpr int NUM_CLUMP_MODEL_INFOS = eMaxArchives::MAX_ANIMOBJECT_ARCHIVES;

    static void InstallModelPatches();
    static void Initialise();

    static CBaseModelInfo* GetModelInfo(int index);
    static CBaseModelInfo* GetModelInfo(const char* name, int start, int end);
    static void SetModelInfo(int index, CBaseModelInfo* info);

    static CAtomicModelInfo* AddAtomicModel(int index);
    static CPedModelInfo* AddPedModel(int index);
    static CClumpModelInfo* AddClumpModel(int index);

private:
    static bool ms_modelPatchesVerified;
    static CBaseModelInfo** ms_modelInfoPtrs;
};