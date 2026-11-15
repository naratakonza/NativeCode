#pragma once

// #include "../../Syncs/CLocalPlayer.h"
// #include "../../Syncs/CRemotePlayer.h"

class CPlayerPool
{
private:
    // CLocalPlayer* m_pLocalPlayer;
    int PlayersCount;

public:
	CPlayerPool();
	~CPlayerPool();

    void Process();

    // CLocalPlayer* GetLocalPlayer() {
    //     return CPlayerPool::m_pLocalPlayer;
    // }
};