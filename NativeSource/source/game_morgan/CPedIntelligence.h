#pragma once

#include "Tasks/CTaskManager.h"

struct CPedGta;

class CPedIntelligence
{
public:
	CPedGta* m_pPed;
	CTaskManager   m_taskManager;
	uint8_t skip[984];

public:
	static void FlushImmediately(CPedIntelligence *pPed, const bool a2);
	static void SetTaskDuckSecondary(CPedIntelligence *thiz, uint16_t nTime);
	static void ClearTaskDuckSecondary(CPedIntelligence *thiz);
};
static_assert(sizeof(CPedIntelligence) == 0x440);
static_assert(offsetof(CPedIntelligence, m_taskManager) == 0x08);