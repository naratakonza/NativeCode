#include "../../../../../Main.h"
#include "CPlayerPool.h"
#include "../../Guis/CGui.h"
#include "../../CMultiPlayer.h"
#include "../../Core/CMultiPlayerPacket.h"
#include "../../../CGame.h"
#include "../../../CUtil.h"

#include <vector>

CPlayerPool::CPlayerPool()
{
    CNecessary::Log(OBFUSCATE("CPlayerPool Initialise"));

    CPlayerPool::m_pLocalPlayer = new CLocalPlayer();
    CPlayerPool::PlayersCount = 0;

    // if (CGui::GetChat()) 
    // {      
    //     CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("CPlayerPool Initialise"));
    // }
}

CPlayerPool::~CPlayerPool()
{
    CNecessary::Log(OBFUSCATE("CPlayerPool Uninitialise"));

    SAFE_DELETE(CPlayerPool::m_pLocalPlayer);
}

void CPlayerPool::Process()
{
    if (CPlayerPool::GetLocalPlayer())
    {
        try
        {
            CPlayerPool::GetLocalPlayer()->Process();
        }
        catch (const std::exception& error)
        {
            CExceptions::Print(OBFUSCATE("s002"), error.what());
        }
        catch (...)
        {
            CExceptions::Print(OBFUSCATE("s002"), OBFUSCATE("unknown C++ exception"));
        }
    }

    std::vector<CRemotePlayer*> playersToRemove;

    for (const auto& playerPair : CPlayerPool::m_pPlayers)
    {
        [[maybe_unused]] uint16_t playerId = playerPair.first;
        CRemotePlayer* pPlayer = playerPair.second;

        if (pPlayer)
        {
            try
            {
                pPlayer->Process();
            }
            catch (...)
            {
                CExceptions::Print(OBFUSCATE("s001"), OBFUSCATE("s001"));
                playersToRemove.push_back(pPlayer);
            }
        }
    }

    for (CRemotePlayer* pPlayer : playersToRemove)
    {
        pPlayer->RemoveGamePlayer(4);
    }
}

void CPlayerPool::DeleteAll()
{
    for (auto playerIter = CPlayerPool::m_pPlayers.begin(); playerIter != CPlayerPool::m_pPlayers.end();)
    {
        CPlayerPool::EjectFromPlayer(playerIter->first);

        SAFE_DELETE(playerIter->second);
        playerIter = CPlayerPool::m_pPlayers.erase(playerIter);
    }
    
    CPlayerPool::PlayersCount = 0;
    CPlayerPool::m_bPlayersSlot.clear();
}

bool CPlayerPool::Add(uint16_t playerID, char* playerName, bool isNPC, uint32_t markerColor)
{
    if (playerID >= eMaxItem::MAX_PLAYERS) {
        return false;
    }

    if (CPlayerPool::m_pPlayers.find(playerID) != CPlayerPool::m_pPlayers.end()) {
        return false;
    }

    CRemotePlayer* newPlayer = new (std::nothrow) CRemotePlayer(playerName, playerID, isNPC);
    if (!newPlayer) {
        return false;
    }

    CPlayerPool::m_pPlayers[playerID] = newPlayer;
    CPlayerPool::m_bPlayersSlot[playerID] = true;

    if (!isNPC)
    {
        CPlayerPool::PlayersCount++;
    }

    CPlayerPool::UpdatePlayerInfo(playerID, markerColor);

    return true;
}

bool CPlayerPool::Delete(uint16_t playerID, [[maybe_unused]] uint8_t byteReason)
{
    if (playerID >= eMaxItem::MAX_PLAYERS) {
        return false;
    }

    auto playerIter = CPlayerPool::m_pPlayers.find(playerID);
    if (playerIter != CPlayerPool::m_pPlayers.end()) 
    {
        CPlayerPool::EjectFromPlayer(playerID);

        SAFE_DELETE(playerIter->second);
        CPlayerPool::m_pPlayers.erase(playerIter);
        CPlayerPool::m_bPlayersSlot[playerID] = false;
        CPlayerPool::PlayersCount--;

        // if (CGui::GetChat()) 
        // {      
        //     CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("DeletePlayer %d"), CPlayerPool::PlayersCount + 1);
        // }

        // CNecessary::Log(OBFUSCATE("DeletePlayer %d"), CPlayerPool::PlayersCount);

        return true;
    }

    return false;
}

void CPlayerPool::EjectFromPlayer(uint16_t playerId)
{
    if (CPlayerPool::GetLocalPlayer()->IsSpectating() && CPlayerPool::GetLocalPlayer()->m_SpectateID == playerId) 
    {
        CPlayerPool::GetLocalPlayer()->EnableSpectating(false);
    }
}

void CPlayerPool::UpdatePlayerInfo(int playerId, uint32_t color)
{
	CRemotePlayer* pPlayer = CPlayerPool::GetRemotePlayer(playerId);
    if (pPlayer)
    {
        if (color != 0)
        {
            pPlayer->UpdateColorOnRadar(color);
        }
    }

    // if (CGui::GetChat()) 
	// {      
	// 	CGui::GetChat()->AddDebugMessage(0, OBFUSCATE("AddPlayer %d"), CPlayerPool::PlayersCount + 1);
	// }

    //CNecessary::Log(OBFUSCATE("AddPlayer %d"), CPlayerPool::PlayersCount);
}

int CPlayerPool::GetPlayersCount()
{
    return CPlayerPool::PlayersCount;
}

void CPlayerPool::DeactivateAll()
{
    if (CPlayerPool::GetLocalPlayer())
    {
		CPlayerPool::GetLocalPlayer()->Deactivate();
	}

    for (const auto& playerPair : CPlayerPool::m_pPlayers)
    {
        CRemotePlayer* pPlayer = playerPair.second;

        if (pPlayer)
        {
            pPlayer->Deactivate();
        }
    }
}

uint16_t CPlayerPool::FindRemotePlayerIDFromGtaPtr(CPedGta* pActor)
{
    if (!pActor) {
        return eInvalidId::INVALID_PLAYER_ID;
    }

    for (const auto& playerPair : CPlayerPool::m_pPlayers)
    {
        uint16_t playerId = playerPair.first;
        CRemotePlayer* pPlayer = playerPair.second;

        if (pPlayer) 
        {
            CPlayerPed* pGamePlayer = pPlayer->GetGamePlayer();
            if (pGamePlayer)
            {
                CPedGta* pTestActor = pGamePlayer->GetPed();
                if (pTestActor && pTestActor == pActor) 
                {
                    return playerId;
                }
            }
        }
    }

    return eInvalidId::INVALID_PLAYER_ID;
}

uint16_t CPlayerPool::FindRemotePlayerIDFromCameraRay(float maxDistance, CVector* cameraPos, CVector* rayEnd, [[maybe_unused]] float radius)
{
    float closestOffset = 100000.0f;
    uint16_t closestPlayerId = eInvalidId::INVALID_PLAYER_ID;

    for (const auto& playerPair : CPlayerPool::m_pPlayers)
    {
        uint16_t playerId = playerPair.first;
        CRemotePlayer* pPlayer = playerPair.second;
        
        if (pPlayer)
        {
            CPlayerPed* pGamePlayer = pPlayer->GetGamePlayer();
            if (pGamePlayer && pGamePlayer->VerifyInstance() && pGamePlayer->IsAdded() && pGamePlayer->GetDistanceFromCamera() < maxDistance)
            {
                CVector boundCenter;
                pGamePlayer->GetBoundCentre(&boundCenter);

                if (pGamePlayer->IsPointInsideEntity(cameraPos, rayEnd, &boundCenter, 1.5f, true))
                {
                    float fOffsetFromRay = CUtil::GetDistanceBetweenPointAndLine(&boundCenter, cameraPos, rayEnd);
                    if (fOffsetFromRay < closestOffset)
                    {
                        closestOffset = fOffsetFromRay;
                        closestPlayerId = playerId;
                    }
                }
            }
        }
    }

    return closestPlayerId;
}

uint16_t CPlayerPool::FindRemotePlayerIDFromGtaEntity(CEntityGta* pEntity)
{
    if (!pEntity) {
        return eInvalidId::INVALID_PLAYER_ID;
    }

    for (const auto& playerPair : CPlayerPool::m_pPlayers)
    {
        uint16_t playerId = playerPair.first;
        CRemotePlayer* pPlayer = playerPair.second;

        if (pPlayer) 
        {
            CPlayerPed* pGamePlayer = pPlayer->GetGamePlayer();
            if (pGamePlayer && pGamePlayer->m_pEntity == pEntity)
            {
                return playerId;
            }
        }
    }

    return eInvalidId::INVALID_PLAYER_ID;
}

void CPlayerPool::ApplyCollisionChecking()
{
    for (const auto& playerPair : CPlayerPool::m_pPlayers)
    {
        CRemotePlayer* pPlayer = playerPair.second;
        if (pPlayer) 
        {
            CPlayerPed* pGamePlayer = pPlayer->GetGamePlayer();
            if (pGamePlayer && !pGamePlayer->IsInVehicle()) 
            {
                pPlayer->SetCollisionCheckingState(pGamePlayer->GetCollisionChecking());
                pGamePlayer->SetCollisionChecking(true);
            }
        }
    }
}

void CPlayerPool::ResetCollisionChecking()
{
    for (const auto& playerPair : CPlayerPool::m_pPlayers)
    {
        CRemotePlayer* pPlayer = playerPair.second;
        if (pPlayer)
        {
            CPlayerPed* pGamePlayer = pPlayer->GetGamePlayer();
            if (pGamePlayer && !pGamePlayer->IsInVehicle()) 
            {
                pGamePlayer->SetCollisionChecking(pPlayer->GetCollisionCheckingState());
            }
        }
    }
}

bool CPlayerPool::IsPlayerNPC(uint16_t playerId)
{
	if (playerId < 0 || playerId >= eMaxItem::MAX_PLAYERS) {
        return false;
    }

	CRemotePlayer* pPlayer = CPlayerPool::GetRemotePlayer(playerId);
	if (pPlayer)
    {
		return pPlayer->IsRemoteNPC();
	}

	return false;
}

void CPlayerPool::UpdateScore(uint16_t playerId, int score)
{
	if (playerId < 0 || playerId >= eMaxItem::MAX_PLAYERS || !CPlayerPool::GetLocalPlayer()) {
        return;
    }

	bool bLocalPlayer = false;
	if (playerId == CPlayerPool::GetLocalPlayer()->GetLocalID()) 
    {
		bLocalPlayer = true;
	}

	if (bLocalPlayer)
    {
		CPlayerPool::GetLocalPlayer()->SetLocalScore(score);
	} 
    else 
    {
		CRemotePlayer* pPlayer = CPlayerPool::GetRemotePlayer(playerId);

		if (pPlayer)
        {
			pPlayer->SetRemoteScore(score);
		}
	}
}

void CPlayerPool::UpdatePing(uint16_t playerId, int ping)
{
	if (playerId < 0 || playerId >= eMaxItem::MAX_PLAYERS || !CPlayerPool::GetLocalPlayer()) {
        return;
    }

	bool bLocalPlayer = false;
	if (playerId == CPlayerPool::GetLocalPlayer()->GetLocalID()) 
    {
		bLocalPlayer = true;
	}

	if (bLocalPlayer) 
    {
		CPlayerPool::GetLocalPlayer()->SetLocalPing(ping);
	} 
    else 
    {
		CRemotePlayer* pPlayer = CPlayerPool::GetRemotePlayer(playerId);

		if (pPlayer) 
        {
			pPlayer->SetRemotePing(ping);
		}
	}
}

bool CPlayerPool::IsValidPointer(uint16_t PlayerID, CRemotePlayer* pPlayer)
{
    auto playerIter = CPlayerPool::m_pPlayers.find(PlayerID);
    if (playerIter == CPlayerPool::m_pPlayers.end() || playerIter->second != pPlayer)
    {
        // CExceptions::Print(OBFUSCATE("PlayerProcess"), OBFUSCATE("PlayerProcess"));
        return false;
    }

    return true;
}