#include "../../Main.h"
#include "CPools.h"

#include <array>
#include <mach-o/dyld.h>
#include <memory>
#include <new>

static bool poolsInstalled = false;

void CPools::Install()
{
    poolsInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x155360), "\x69\x07\x0D\x94") && CMemory::HasMappedInstruction(SA_ADDR(0x16CE18), "\xE9\x23\xBD\x6D") && CMemory::HasMappedInstruction(SA_ADDR(0x16CEA4), "\x13\xE1\x07\xF9");
    if (!poolsInstalled)
    {
        CNecessary::Log(OBFUSCATE("CPools: iOS startup bridge unavailable"));
    }
}

CPools* CPools::Allocate(std::size_t count, std::size_t stride)
{
    auto pool = std::make_unique<CPools>();
    std::unique_ptr<std::uint8_t[]> objects(new std::uint8_t[count * stride]);
    std::unique_ptr<std::uint8_t[]> flags(new std::uint8_t[count]);
    std::memset(flags.get(), 0x80, count);

    pool->objects = objects.release();
    pool->flags = flags.release();
    pool->count = static_cast<int>(count);
    pool->top = -1;
    pool->bOwnsAllocations = true;
    
    // เอนจินจะปลดปล่อยสิ่งเหล่านี้ด้วยตัวดำเนินการ delete[] ภายในฟังก์ชัน CPools::ShutDown
    return pool.release();
}

void CPools::Initialise(void (*original)())
{
    if (!poolsInstalled)
    {
        original();
        return;
    }

    struct PoolSize {
        std::size_t count;
        std::size_t stride;
    };

    // เรียงลำดับเช่นเดียวกับ iOS pool globals จำนวน 17 รายการที่อยู่ติดกัน
    static constexpr std::array<PoolSize, 17> sizes{{
        {100000, 16}, // PtrNodeSingleLink
        {60000, 24},  // PtrNodeDoubleLink
        {20000, 40},  // EntryInfoNode
        {240, 2504},  // Ped
        {2000, 3176}, // Vehicle
        {20000, 96},  // Building
        {3000, 544},  // Object
        {40000, 96},  // Dummy
        {50000, 56},  // ColModel
        {5000, 168},  // Task
        {1000, 88},   // Event
        {200, 100},   // PointRoute
        {200, 420},   // PatrolRoute
        {200, 36},    // NodeRoute
        {3000, 48},   // TaskAllocator
        {240, 1088},  // PedIntelligence
        {200, 236},   // PedAttractor
    }};

    std::array<CPools*, sizes.size()> pending{};
    try
    {
        for (std::size_t i = 0; i < sizes.size(); ++i)
        {
            pending[i] = CPools::Allocate(sizes[i].count, sizes[i].stride);
        }
    }
    catch (const std::bad_alloc&)
    {
        for (auto* pool : pending)
        {
            if (pool)
            {
                delete[] pool->objects;
                delete[] pool->flags;
                delete pool;
            }
        }

        CNecessary::Log(OBFUSCATE("CPools: expanded allocation failed, using GTA pools"));
        original();
        return;
    }

    auto** slots = reinterpret_cast<CPools**>(SA_ADDR(0x734FC0) + _dyld_get_image_vmaddr_slide(0));
    std::copy(pending.begin(), pending.end(), slots);
    CNecessary::Log(OBFUSCATE("CPools Initialise"));
}

extern "C" __attribute__((visibility("default"))) void CPools_Initialise_hook(void (*original)())
{
    CPools::Initialise(original);
}