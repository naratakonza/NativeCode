#pragma once

#include "../Enums/eMaxItem.h"

#include <cstddef>
#include <cstdint>

// Zero-node fallback ของ Thunder; hierarchy ฝั่ง iOS ใช้พื้นที่ถึง offset 0x20.
struct CCrashFixAnimHierarchy
{
    std::uint64_t name;
    void* sequences;
    std::uint8_t state[0x18];
};

struct CCrashFixAnimAssociation
{
    std::uint8_t state[0x18];
    CCrashFixAnimHierarchy* hierarchy;
};

static_assert(sizeof(CCrashFixAnimHierarchy) == 0x28);
static_assert(offsetof(CCrashFixAnimHierarchy, sequences) == 0x08);
static_assert(sizeof(CCrashFixAnimAssociation) == 0x20);
static_assert(offsetof(CCrashFixAnimAssociation, hierarchy) == 0x18);

class CCrashFix
{
public:
	static void Initialise();

};