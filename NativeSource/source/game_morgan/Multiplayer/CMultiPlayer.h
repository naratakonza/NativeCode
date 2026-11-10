#pragma once

#include "../Enums/eThreadType.h"

class CNetGame;

class CMultiPlayer
{
public:
    static bool m_bPlaySAMP;
    static bool m_bGameStarted;
	static bool m_bSAMPInited;

public:
	static void Initialise();
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

private:
	static CNetGame* pNetGame;
};