#include "CEntity.h"
#include "../CCommon.h"
#include "../CStreaming.h"
#include "../Multiplayer/Guis/CGui.h"
#include "../Multiplayer/CMultiPlayer.h"
#include "../Enums/eSpecialVehicle.h"
#include "../Models/CModelInfo.h"
#include "../Scripts/CScripting.h"
#include "../../utils/Necessary/CNecessary.h"

#include <pthread.h>
#include <unistd.h>

void CEntity::Add()
{

}

void CEntity::Remove()
{

}

void CEntity::TeleportTo(float x, float y, float z)
{
    if (!pthread_main_np() || !m_pEntity || !m_pEntity->vtable || CCommon::IsGameEntityArePlaceable(m_pEntity)) {
        return;
    }

    if (!m_pEntity->mat) {
        return;
    }

    const uint16_t modelIndex = m_pEntity->nModelIndex;
    if (modelIndex == eSpecialVehicle::TRAIN_PASSENGER_LOCO || modelIndex == eSpecialVehicle::TRAIN_FREIGHT_LOCO || modelIndex == eSpecialVehicle::TRAIN_TRAM)
    {
        CScripting::ScriptCommand(&put_train_at, m_dwGTAId, x, y, z);
    }
    else
    {
        const auto teleportEntity = *reinterpret_cast<void (*const *)(CEntityGta*, CVector, uint8_t)>(m_pEntity->vtable + 0x78);
        teleportEntity(m_pEntity, {x, y, z}, 0);
    }
}

bool CEntity::SetModelIndex(unsigned int uiModel)
{
    if (!pthread_main_np() || !m_pEntity || !m_pEntity->vtable) {
        return false;
    }

    int iWaitPreloaded = 0;

    if (!CStreaming::IsModelLoaded(uiModel))
    {
        CStreaming::RequestModel(uiModel);
        CStreaming::LoadAllRequestedModels(false);
        while (!CStreaming::IsModelLoaded(uiModel))
        {
            usleep(1000);
            iWaitPreloaded++;
            if (iWaitPreloaded > 200)
            {
                if (CGui::GetChat())
                {
                    CGui::GetChat()->AddDebugMessage(1, OBFUSCATE("Warning: Model %u wouldn't load in time!"), uiModel);
                }

                return false;
            }
        }
    }

    // DeleteRWObject(); // เหมือนจะส่งผล Crash ใน 64Bit เยอะเลย

    m_pEntity->nModelIndex = static_cast<uint16_t>(uiModel);

    // CEntity::SetModelIndex()
    (*reinterpret_cast<void (*const *)(CEntityGta*, unsigned int)>(m_pEntity->vtable + 0x30))(m_pEntity, uiModel);

    return true;
}
