#pragma once

#include "../../../Core/CVector.h"

struct CheckpointData
{
    bool bCheckpointsEnabled;
    uint32_t dwCheckpointMarker;
    uint32_t dwCheckpointSphere;
    CVector vecCheckpointPos;
    CVector vecCheckpointExtent;
};

struct RaceCheckpointData
{
    bool bRaceCheckpointsEnabled;
    uint32_t dwRaceCheckpointHandle;
    uint32_t dwRaceCheckpointMarker;
    float fRaceCheckpointSize;
    uint8_t byteRaceType;
    CVector vecRaceCheckpointPos;
    CVector vecRaceCheckpointNext;
};

class CCheckPoint
{
private:
    static CheckpointData m_checkpointData;
    static RaceCheckpointData m_raceCheckpointData;

public:
    static void Initialise();
    static void Create(CVector *pos, CVector *extent);
    static void CreateRace(int8_t byteType, CVector *pos, CVector *next, float fSize);
    static void DisableRace();
    static void Process();
    static void ToggleCheckpoints(bool bEnabled);
    static void ToggleRaceCheckpoints(bool bEnabled);
};