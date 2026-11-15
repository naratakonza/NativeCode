#pragma once

#include "../../Enums/eSpawnAction.h"

class CSpawnScreen
{
public:
    CSpawnScreen();
    ~CSpawnScreen();

    void Enable(bool status);
    bool GetActiveSpawnScreen();
    bool HandleAction(eSpawnAction action);

public:
    bool m_bRenderStatus;
    bool m_bSendSpawn;
    bool m_bSendPrev;
    bool m_bSendNext;
};