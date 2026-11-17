#pragma once

#include "CTask.h"

struct CPedGta;

enum ePrimaryTasks
{
    TASK_PRIMARY_PHYSICAL_RESPONSE = 0,
    TASK_PRIMARY_EVENT_RESPONSE_TEMP,
    TASK_PRIMARY_EVENT_RESPONSE_NONTEMP,
    TASK_PRIMARY_PRIMARY,
    TASK_PRIMARY_DEFAULT,
    TASK_PRIMARY_MAX
};

enum eSecondaryTask : uint32_t
{
    TASK_SECONDARY_INVALID = (uint32_t)-1,

    TASK_SECONDARY_ATTACK = 0,
    TASK_SECONDARY_DUCK,
    TASK_SECONDARY_SAY,
    TASK_SECONDARY_FACIAL_COMPLEX,
    TASK_SECONDARY_PARTIAL_ANIM,
    TASK_SECONDARY_IK,
    TASK_SECONDARY_MAX
};

class CTaskManager
{
public:
	CTask* m_tasks[ePrimaryTasks::TASK_PRIMARY_MAX];
	CTask* m_aSecondaryTasks[eSecondaryTask::TASK_SECONDARY_MAX];
	CPedGta* m_pPed;
};
static_assert(sizeof(CTaskManager) == 0x60);
static_assert(offsetof(CTaskManager, m_aSecondaryTasks) == 0x28);
static_assert(offsetof(CTaskManager, m_pPed) == 0x58);