#include "../../Main.h"
#include "CStats.h"

#include <mach-o/dyld.h>

float (*CStats::StatTypesFloat)[82] = reinterpret_cast<float(*)[82]>(SA_ADDR(0x73AFDC) + _dyld_get_image_vmaddr_slide(0));

static bool statsHookInstalled = false;

extern "C" __attribute__((visibility("default"))) float CStats_GetFatAndMuscleModifier_hook(eStatModAbilities nAbility, float (*original)(eStatModAbilities))
{
    if (statsHookInstalled)
    {
        switch (nAbility)
        {
            case eStatModAbilities::STAT_MODIFIER_MAX_HEALTH: {
                return ((*CStats::StatTypesFloat)[24] / 1000.0f) * 176.0f;
            }
            
            case eStatModAbilities::STAT_MODIFIER_MAX_HEALTH_LIMIT: {
                return 176.0f;
            }

            default: {
                break;
            }
        }
    }

    return original(nAbility);
}

void CStats::Initialise()
{
    CNecessary::Log(OBFUSCATE("CStats Initialise"));

    statsHookInstalled = CMemory::HasMappedBytes(SA_ADDR(0x17A3E8), "\x74\x74\x0C\x14\xE8\x19\x00\x54", 8, "__text") && CMemory::HasMappedBytes(SA_ADDR(0x4975B8), "\x81\x00\x00\x10\xF0\x00\x00\xD0\x10\x6E\x42\xF9\x00\x02\x1F\xD6\x1F\x3C\x00\x71\x88\x8B\xF3\x17", 24, "__mg_gameproc") && CMemory::HasMappedBytes(SA_ADDR(0x1778CC), "\x09\x2E\x00\xF0\x29\x71\x3F\x91\x20\x79\x68\xBC", 12, "__text");
    if (!statsHookInstalled)
    {
        CNecessary::Log(OBFUSCATE("CStats GetFatAndMuscleModifier bridge unavailable"));
    }
}