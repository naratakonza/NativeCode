#pragma once

#include "CNetGame.h"
#include "../Enums/eThreadType.h"

#define NETGAME_VERSION 4057
#define AUTH_BS_TH OBFUSCATE("15121F6F18550C00AC4B4F8A167D0379BB0ACA99043") // 2.0

class CMultiPlayer
{
public:
    static bool m_bPlaySAMP;
    static bool m_bGameStarted;
	static bool m_bSAMPInited;

public:
	static void Initialise();
    static void Reconnect(bool waitconnect = true);
    static void ChangeServer(const char* ip, int port);
	static void Uninitialise();
	static void Process();
	static void StartSAMP();
	static void SetThread(eThreadType type);

	static CNetGame* GetActiveNetwork()
	{
		return CMultiPlayer::pNetGame;
	}

	static bool IsSampInit()
    {
        return CMultiPlayer::m_bSAMPInited;
    }

	static bool IsGameActive()
    {
        return CMultiPlayer::m_bGameStarted;
    };

	static bool IsSampActive()
    {
        return CMultiPlayer::m_bPlaySAMP;
    }

	static const char* GetPlayerName()
    {
        return "Morgan_Artiwliz";
    }

private:
	static CNetGame* pNetGame;
};