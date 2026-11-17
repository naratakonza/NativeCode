#pragma once

#include <cstddef>
#include <cstdint>

class CTask {
public:
    uintptr_t vtable;
    CTask* m_pParent;
};
static_assert(sizeof(CTask) == 0x10);
static_assert(offsetof(CTask, m_pParent) == 0x08);