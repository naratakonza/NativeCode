#pragma once

#include "../../Syncs/CLocalPlayer.h"
#include "../../Syncs/CRemotePlayer.h"

class CPlayerPool
{
private:
    CLocalPlayer* m_pLocalPlayer;
    std::unordered_map<uint16_t, CRemotePlayer*> m_pPlayers;
    std::unordered_map<uint16_t, bool> m_bPlayersSlot;
    int PlayersCount;

public:
	CPlayerPool();
	~CPlayerPool();

    void Process();
    void DeleteAll();
    bool Add(uint16_t playerID, char* playerName, bool isNPC = false, uint32_t markerColor = 0);
    bool Delete(uint16_t playerID, uint8_t byteReason);
    void EjectFromPlayer(uint16_t playerId);
    void UpdatePlayerInfo(int playerId, uint32_t color = 0);
    int GetPlayersCount();
    void DeactivateAll();

    CLocalPlayer* GetLocalPlayer() {
        return CPlayerPool::m_pLocalPlayer;
    }

    std::unordered_map<uint16_t, CRemotePlayer*>& GetAllRemotePlayers() {
        return CPlayerPool::m_pPlayers;
    }

    CRemotePlayer* GetRemotePlayer(uint16_t id) {
        auto it = CPlayerPool::m_pPlayers.find(id);
        if (it != CPlayerPool::m_pPlayers.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool GetRemotePlayerInSlot(uint16_t id) {
        auto it = CPlayerPool::m_bPlayersSlot.find(id);
        if (it != CPlayerPool::m_bPlayersSlot.end()) {
            return it->second;
        }
        
        return false;
    }

    CObject* GetAccessoryFromGtaPtr(CEntityGta* pEntity);
    uint16_t FindRemotePlayerIDFromGtaPtr(CPedGta* pActor);
    uint16_t FindRemotePlayerIDFromGtaEntity(CEntityGta* pEntity);
    uint16_t FindRemotePlayerIDFromCameraRay(float maxDistance, CVector* cameraPos, CVector* rayEnd, float radius);
    void ApplyCollisionChecking();
    void ResetCollisionChecking();
    bool IsPlayerNPC(uint16_t playerId);
    void UpdateScore(uint16_t playerId, int score);
    void UpdatePing(uint16_t playerId, int score);
    uint16_t FindNearestToLocalPlayerPed();
    bool IsValidPointer(uint16_t PlayerID, CRemotePlayer* pPlayer);

    void ProcessAllPlayersAfterPreRender();
    void ProcessAllPlayersAttachedEntities();
};