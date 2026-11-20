#include "../../../../Main.h"
#include "CTaskSimplePlayerOnFoot.h"
#include "../../CWorld.h"
#include "../../Base/CPad.h"

#include "../../Multiplayer/CMultiPlayer.h"

#include <pthread.h>

static bool playerOnFootWeaponHookInstalled = false;

uintptr_t (*CTaskSimplePlayerOnFoot_ProcessPlayerWeapon)(uintptr_t thiz, CPedGta* pPlayerPed);
extern "C" __attribute__((visibility("default"))) uintptr_t CTaskSimplePlayerOnFoot_ProcessPlayerWeapon_hook(uintptr_t thiz, CPedGta* pPlayerPed, uintptr_t (*original)(uintptr_t, CPedGta*))
{
    if (!playerOnFootWeaponHookInstalled || !pthread_main_np() || !CMultiPlayer::IsSampActive())
    {
        return original(thiz, pPlayerPed);
    }

    CTaskSimplePlayerOnFoot_ProcessPlayerWeapon = original;

    CPad::dwCurWeaponProcessingPlayer = pPlayerPed;
    uintptr_t result;

    if (CPad::m_pCurrentContextOnFoot == Context::REMOTE_PLAYER)
    {
        CWorld::PlayerInFocus = CPad::m_byteCurPlayer;

        result = CTaskSimplePlayerOnFoot_ProcessPlayerWeapon(thiz, CPad::dwCurWeaponProcessingPlayer);

        CWorld::PlayerInFocus = 0;
    }
    else
    {
        result = CTaskSimplePlayerOnFoot_ProcessPlayerWeapon(thiz, CPad::dwCurWeaponProcessingPlayer);
    }

    CPad::dwCurWeaponProcessingPlayer = nullptr;
    return result;
}

void CTaskSimplePlayerOnFoot::Initialize()
{
    CNecessary::Log(OBFUSCATE("CTaskSimplePlayerOnFoot Initialise"));

    playerOnFootWeaponHookInstalled = CMemory::HasMappedInstruction(SA_ADDR(0x3BECD0), "\xBC\x63\x03\x14");
}