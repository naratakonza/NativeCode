#include "../../../Main.h"
#include "CExceptions.h"
#include "../../game_morgan/Multiplayer/Guis/CGui.h"

void CExceptions::Print(const char* type, const char* detail)
{
    static std::atomic<unsigned int> warnings{0};
    auto count = warnings.load(std::memory_order_relaxed);

    while (count < 10)
    {
        if (warnings.compare_exchange_weak(count, count + 1, std::memory_order_relaxed))
        {
            CNecessary::Log(OBFUSCATE("Warning(%s): %s"), type, detail);

            if (pthread_main_np() && CGui::GetChat())
            {
                CGui::GetChat()->AddDebugMessage(1, OBFUSCATE("Warning(%s): %s"), type, detail);
            }

            return;
        }
    }
}