#include "../../../Main.h"
#include "CRunningScript.h"

#include <mach-o/dyld.h>

static const ScriptCommandSlot* commandTable = nullptr;
static std::uint16_t* commandsExecuted = nullptr;

bool CRunningScript::Initialise()
{
    if (commandTable) {
        return true;
    }

    const bool hasCounter = CMemory::HasMappedInstruction(SA_ADDR(0xDCD44), "\x54\x32\x00\xD0") && CMemory::HasMappedInstruction(SA_ADDR(0xDCD6C), "\x88\x36\x40\x79") && CMemory::HasMappedInstruction(SA_ADDR(0xDCD74), "\x88\x36\x00\x79");
    const bool hasTable = CMemory::HasMappedInstruction(SA_ADDR(0xDCD58), "\xF7\x1D\x00\xD0") && CMemory::HasMappedInstruction(SA_ADDR(0xDCD5C), "\xF7\x22\x2C\x91") && CMemory::HasMappedInstruction(SA_ADDR(0xDCDB0), "\x28\x25\x40\xA9");
    const bool hasOpcode = CMemory::HasMappedInstruction(SA_ADDR(0xDCD7C), "\x09\x25\x40\x78") && CMemory::HasMappedInstruction(SA_ADDR(0xDCD80), "\x68\x12\x00\xF9") && CMemory::HasMappedInstruction(SA_ADDR(0xDCD88), "\x68\x8A\x04\x39") && CMemory::HasMappedInstruction(SA_ADDR(0xDCD98), "\x3F\x2C\x2A\x71");

    if (!hasCounter || !hasTable || !hasOpcode) {
        return false;
    }

    constexpr std::uintptr_t handlerOffsets[] = {
        0xDD1B4, 0xDDB5C, 0xDEF8C, 0xF6844, 0xF7854, 0xF8A2C, 0xF9B30, 0xFA268, 0xFB7BC,
        0xFD868, 0x10472C, 0x105918, 0x107310, 0x108AB4, 0x109E84, 0x10C1B0, 0x10FFC0, 0x11268C,
        0xE2B70, 0xE5018, 0xE910C, 0xEB104, 0xED23C, 0xEEFB0, 0xF09F4, 0xF2CB8, 0xF4BE0,
    };
    
    static_assert(sizeof(handlerOffsets) / sizeof(handlerOffsets[0]) == (MaxOpcode / 100) + 1);

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    const auto* table = reinterpret_cast<const ScriptCommandSlot*>(SA_ADDR(0x49AB08) + slide);

    for (std::size_t i = 0; i < sizeof(handlerOffsets) / sizeof(handlerOffsets[0]); ++i)
    {
        if (table[i].thisAdjustment != 0 || reinterpret_cast<std::uintptr_t>(table[i].handler) != SA_ADDR(handlerOffsets[i]) + slide) {
            return false;
        }
    }

    commandsExecuted = reinterpret_cast<std::uint16_t*>(SA_ADDR(0x72601A) + slide);
    commandTable = table;
    return true;
}

void CRunningScript::ProcessOneCommand(CRunningScript* thiz)
{
    if (!commandTable || !thiz || !thiz->m_pPCPointer) {
        return;
    }

    std::uint16_t encoded;
    std::memcpy(&encoded, thiz->m_pPCPointer, sizeof(encoded));
    const auto opcode = encoded & 0x7FFF;

    if (opcode > MaxOpcode) {
        return;
    }

    ++*commandsExecuted;
    thiz->m_pPCPointer += sizeof(encoded);
    thiz->m_bNotForLatestExpression = static_cast<std::uint8_t>(encoded >> 15);
    commandTable[opcode / 100].handler(thiz, opcode);
}