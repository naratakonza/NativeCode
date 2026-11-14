#pragma once

#include "CRect.h"

#include <cstddef>
#include <cstdint>

class CEntryExit {
public:
    uint8_t cTitle[8];                // 0x00
    CRect rect;                       // 0x08
    float pz;                         // 0x18
    float prot;                       // 0x1C
    float spawnx;                     // 0x20
    float spawny;                     // 0x24
    float spawnz;                     // 0x28
    float spawnrot;                   // 0x2C
    uint16_t flags;                   // 0x30
    uint8_t areacode;                 // 0x32
    uint8_t extracol;                 // 0x33
    uint8_t openTime;                 // 0x34
    uint8_t shutTime;                 // 0x35
    int8_t NumRandomPedsToCreate;     // 0x36
    uint8_t pad01;                    // 0x37
    CEntryExit* pLink;                // 0x38
};
static_assert(offsetof(CEntryExit, flags) == 0x30);
static_assert(sizeof(CEntryExit) == 0x40);