#include "../../../../Main.h"
#include "CLocalPlayer.h"
#include "../../CGame.h"
#include "../CMultiPlayer.h"
#include "../Guis/CGui.h"
#include "../../Scripts/CScripting.h"

uint32_t dwEnterVehTimeElasped = -1;
bool bFirstSpawn = true;

CLocalPlayer::CLocalPlayer()
{
    CNecessary::Log(OBFUSCATE("CLocalPlayer Initialise"));

    CLocalPlayer::m_pPlayerPed = CGame::GetPlayer();

    if (CGui::GetChat())
    {
        CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("CLocalPlayer Initialise"));
    }
}

CLocalPlayer::~CLocalPlayer()
{
    CNecessary::Log(OBFUSCATE("CLocalPlayer Uninitialise"));

    CLocalPlayer::m_pPlayerPed = nullptr;
}

void CLocalPlayer::ResetData()
{
	
}

bool CLocalPlayer::Process()
{

	return true;
}