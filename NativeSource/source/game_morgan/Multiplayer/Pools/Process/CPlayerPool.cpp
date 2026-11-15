#include "../../../../../Main.h"
#include "CPlayerPool.h"
#include "../../Guis/CGui.h"

CPlayerPool::CPlayerPool()
{
    CNecessary::Log(OBFUSCATE("CPlayerPool Initialise"));

    //CPlayerPool::m_pLocalPlayer = new CLocalPlayer();
    CPlayerPool::PlayersCount = 0;

    if (CGui::GetChat()) 
    {      
        CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("CPlayerPool Initialise"));
    }
}

CPlayerPool::~CPlayerPool()
{
    CNecessary::Log(OBFUSCATE("CPlayerPool Uninitialise"));

    // CPlayerPool::DeleteAll();
    // SAFE_DELETE(CPlayerPool::m_pLocalPlayer);
}

void CPlayerPool::Process()
{
    
}