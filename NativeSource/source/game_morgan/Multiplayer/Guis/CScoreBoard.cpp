#include "../../../../Main.h"
#include "CScoreBoard.h"
#include "CGui.h"
#include "../CMultiPlayer.h"
#include "../../CGame.h"
#include "../../../../Launcher/MGFlutter.h"

#include "../Core/CMultiPlayerPacket.h"

static std::mutex scoreBoardActionsMutex;
static std::vector<int> scoreBoardActions;

CScoreBoard::CScoreBoard()
{
    CScoreBoard::m_bHasActiveScoreBoard = false;
    std::lock_guard<std::mutex> lock(scoreBoardActionsMutex);
    scoreBoardActions.clear();
}

CScoreBoard::~CScoreBoard()
{
    CScoreBoard::Close();
    std::lock_guard<std::mutex> lock(scoreBoardActionsMutex);
    scoreBoardActions.clear();
}

void CScoreBoard::Draw()
{
    if (CMultiPlayer::GetActiveNetwork())
    {
        if (CMultiPlayer::GetActiveNetwork()->GetNetworkState() != eNetworkState::GAMESTATE_CONNECTED) {
            return;
        }

        if (CGui::GetDialog()->GetActiveDialog()) {
            return;
        }

        CScoreBoard::Enable(true);
        CGame::DisplayHUD(false);

        MGSetFlutterScoreBoardVisible(true);
    }
}

void CScoreBoard::Close()
{
    MGSetFlutterScoreBoardVisible(false);

    CScoreBoard::Enable(false);
    CGame::DisplayHUD(true);
}

void CScoreBoard::Enable(bool Enable)
{
    CScoreBoard::m_bHasActiveScoreBoard = Enable;
}

void CScoreBoard::SendNotification(int ID)
{
    CMultiPlayerPacket::SendTabClickPlayer(ID);
}

bool CScoreBoard::QueueAction(int playerId)
{
    std::lock_guard<std::mutex> lock(scoreBoardActionsMutex);
    if (scoreBoardActions.size() >= 64) {
        return false;
    }

    scoreBoardActions.push_back(playerId);
    return true;
}

void CScoreBoard::ProcessPendingActions()
{
    std::vector<int> actions;
    {
        std::lock_guard<std::mutex> lock(scoreBoardActionsMutex);
        actions.swap(scoreBoardActions);
    }

    for (int playerId : actions)
    {
        if (playerId >= 0)
        {
            CScoreBoard::SendNotification(playerId);
        }

        CScoreBoard::Close();
    }
}

void CScoreBoard::SetTabStat(int id, const char* name, int score, int ping, uint32_t color)
{
    char utf8Name[256]{};
    CNecessary::ConvertLocalizedEncodingToUTF8(utf8Name, name, sizeof(utf8Name));
    MGUpdateFlutterScoreBoardPlayer(id, utf8Name, score, ping, color);
}

void CScoreBoard::TabInFo(const char* serverName, int online)
{
    char utf8Name[256]{};
    CNecessary::ConvertLocalizedEncodingToUTF8(utf8Name, serverName, sizeof(utf8Name));
    if (std::strlen(utf8Name) > 70)
    {
        size_t length = 70;
        while ((static_cast<unsigned char>(utf8Name[length]) & 0xC0) == 0x80)
        {
            --length;
        }

        std::strcpy(utf8Name + length, "...");
    }

    MGSetFlutterScoreBoardInfo(utf8Name, online);
}

void CScoreBoard::SetLocalPlayerID(int id)
{
    MGSetFlutterScoreBoardLocalPlayer(id);
}