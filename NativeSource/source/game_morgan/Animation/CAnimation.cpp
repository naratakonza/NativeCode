#include "../../../Main.h"
#include "CAnimation.h"
#include "../Core/CKeyGen.h"
#include "../CCommon.h"

#include <mach-o/dyld.h>
#include <strings.h>

uint32_t CAnimation::m_AnimId = 0;
uint32_t CAnimation::m_AnimGroup = 0;
float CAnimation::m_BlendDelta = 0.0f;
uint32_t CAnimation::m_dwAnimationKeyTable[1812] = { 0 };

void CAnimation::Initialise()
{
    char *szAnimDelimiter = (char*)nullptr;
    char szAnimLib[32 + 1] = { '\0' };
    char szAnimName[32 + 1] = { '\0' };

    for (size_t index = 0; index < 1812; index++)
    {
        memset(szAnimLib, 0, sizeof(szAnimLib));
        memset(szAnimName, 0, sizeof(szAnimName));

        szAnimDelimiter = strchr((char*)m_szAnimNames[index], ':');
        if (!szAnimDelimiter || !CNecessary::MPSTRLEN(szAnimDelimiter)) {
            continue;
        }

        strncpy(szAnimLib, m_szAnimNames[index], (size_t)(szAnimDelimiter - m_szAnimNames[index]));
        strcpy(szAnimName, ++szAnimDelimiter);

        CAnimation::m_dwAnimationKeyTable[index] = CKeyGen::GetUppercaseKey((const char*)szAnimName);
    }
}

int16_t CAnimation::GetAnimationIndexFromKey(int animKey)
{
    for (int i = 0; i < 1812; ++i)
    {
        if (CAnimation::m_dwAnimationKeyTable[i] == static_cast<uint32_t>(animKey))
        {
            return static_cast<int16_t>(i + 1); 
        }
    }

    return -1;
}

int CAnimation::GetAnimationKey(uint32_t group, uint32_t id)
{
    static const bool verified = CMemory::HasMappedBytes(SA_ADDR(0x4230C), "\x88\x32\x00\x90\x08\x99\x40\xF9\xE9\x03\x00\x2A", 12, "__text") || (CMemory::HasMappedInstruction(SA_ADDR(0x4230C), "\x6E\x56\x11\x14") && CMemory::HasMappedBytes(SA_ADDR(0x497CC4), "\x82\x00\x00\x10\xF0\x00\x00\xD0\x10\x56\x43\xF9\x00\x02\x1F\xD6\xC8\x0F\x00\xF0\x8E\xA9\xEE\x17", 24, "__mg_gameproc"));
    if (!verified) {
        return 0;
    }

    uintptr_t animAssoc = reinterpret_cast<uintptr_t (*)(uint32_t, uint32_t)>(SA_ADDR(0x4230C) + _dyld_get_image_vmaddr_slide(0))(group, id);
    if (animAssoc)
    {
        uintptr_t hierarchy = *reinterpret_cast<uintptr_t*>(animAssoc + 0x18);
        if (hierarchy)
        {
            return *reinterpret_cast<int*>(hierarchy);
        }
    }
    
    return 0;
}

const char* CAnimation::RetrieveAnimationByIndex(int idx)
{
    if (idx < 0 || idx >= 1812) {
        return nullptr;
    }
    
    return m_szAnimNames[idx];
}

int16_t CAnimation::GetAnimationIndexByName(const char *szGroupName, const char *szAnimName)
{
    if (!szGroupName || !szAnimName) {
        return -1;
    }

    if (strlen(szGroupName) + strlen(szAnimName) > 0x24) {
        return -1;
    }

    char szBuffer[40] = { 0 };
    snprintf(szBuffer, sizeof(szBuffer), OBFUSCATE("%s:%s"), szGroupName, szAnimName);
    CNecessary::UpperCaseString(szBuffer);

    for (int i = 0; i < 1812; ++i)
    {
        if (strcasecmp(szBuffer, m_szAnimNames[i]) == 0)
        {
            return static_cast<int16_t>(i + 1);
        }
    }

    return -1;
}

uint32_t CAnimation::GetIndexFromTask(void* task)
{
    if (!task) {
        return 0;
    }

    if (!CCommon::IsTaskRunNamedOrSlideToCoord(task)) {
        return 0;
    }

    uintptr_t pTask = reinterpret_cast<uintptr_t>(task);

    const char* szAnimName = reinterpret_cast<const char*>(pTask + 0x19); // pTask->m_animName
    const char* szGroupName = reinterpret_cast<const char*>(pTask + 0x31); // pTask->m_animGroupName

    if (!szAnimName || !szGroupName) {
        return 0;
    }

    int16_t animIndex = CAnimation::GetAnimationIndexByName(szGroupName, szAnimName);
    if (animIndex <= 0) {
        return 0;
    }

    float blendDelta = *reinterpret_cast<float*>(pTask + 0x44); // pTask->m_fBlendDelta
    int time = *reinterpret_cast<int*>(pTask + 0x50); // pTask->m_iTime
    uint8_t taskFlags = *reinterpret_cast<uint8_t*>(pTask + 0x18);

    bool bLoop = (taskFlags & 0x02) != 0;
    bool bLockX = (taskFlags & 0x10) != 0;
    bool bLockY = (taskFlags & 0x20) != 0;
    bool bFreeze = (taskFlags & 0x04) != 0;

    uint8_t loopFlag = bLoop ? 1 : 0;
    uint8_t lockXFlag = bLockX ? 1 : 0;
    uint8_t freezeFlag = bFreeze ? 1 : 0;
    uint8_t lockYFlag = bLockY ? 0 : 1;

    uint8_t timeFlag = 0;
    if (time <= 0)
    {
        timeFlag = (time != 0) ? 1 : 0;
    }
    else
    {
        timeFlag = 2;
    }

    uint8_t blendByte = static_cast<uint8_t>(blendDelta);
    uint32_t flagsByte = loopFlag | (lockXFlag << 1) | (freezeFlag << 2) | (lockYFlag << 3) | (timeFlag << 4);

    return animIndex | (blendByte << 16) | (flagsByte << 24);
}