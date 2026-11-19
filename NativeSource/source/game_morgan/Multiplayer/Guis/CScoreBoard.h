#pragma once

#include <cstdint>

class CScoreBoard
{
public:
    CScoreBoard();
    ~CScoreBoard();

    void Draw();
    void Close();
    void Enable(bool enable);
    void SendNotification(int ID);

    void SetTabStat(int id, const char* name, int score, int ping, uint32_t color);
    void TabInFo(const char* serverName, int online);
    void SetLocalPlayerID(int id);
    void ProcessPendingActions();
    static bool QueueAction(int playerId);

    bool GetActiveScore() {
        return m_bHasActiveScoreBoard;
    }

private:
    bool m_bHasActiveScoreBoard;
};