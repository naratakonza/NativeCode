#pragma once

#include "CVector.h"
#include "../../utils/Memory/CMemory.h"

#include <mach-o/dyld.h>

struct RwMatrixTag;
typedef RwMatrixTag RwMatrix;

class CMatrix
{
public:
    CVector right;        // 0x0  // RW: Right
    uint32_t  flags;          // 0xC
    CVector up;      // 0x10 // RW: Up
    uint32_t  pad1;           // 0x1C
    CVector at;           // 0x20 // RW: At
    uint32_t  pad2;           // 0x2C
    CVector pos;          // 0x30
    uint32_t  pad3;           // 0x3C

    RwMatrix* m_pAttachMatrix{};       // 0x40
    uint32_t  m_bOwnsAttachedMatrix{}; // 0x48 - Do we need to delete attached matrix at detaching
public:
    void UpdateRwMatrix(RwMatrix* rwMatrix)
    {
        static const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x22A654), "\x00\x00\x40\xBD") && CMemory::HasMappedInstruction(SA_ADDR(0x22A658), "\x20\x00\x00\xBD") && CMemory::HasMappedInstruction(SA_ADDR(0x22A6B8), "\x6E\x36\x02\x14");
        if (!verified || !rwMatrix) {
            return;
        }

        reinterpret_cast<void (*)(CMatrix*, RwMatrix*)>(SA_ADDR(0x22A654) + _dyld_get_image_vmaddr_slide(0))(this, rwMatrix);
    }

    CVector& GetPosition() { return pos; }
    const CVector& GetPosition() const { return pos; }
};
static_assert(sizeof(CMatrix) == 0x50);
static_assert(offsetof(CMatrix, pos) == 0x30);
static_assert(offsetof(CMatrix, m_pAttachMatrix) == 0x40);
static_assert(offsetof(CMatrix, m_bOwnsAttachedMatrix) == 0x48);
