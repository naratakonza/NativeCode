#include "../../../../../Main.h"
#include "CCheckPoint.h"
#include "CMarkerIcon.h"
#include "../../Guis/CGui.h"
#include "../../../CGame.h"
#include "../../../Scripts/CScripting.h"

CheckpointData CCheckPoint::m_checkpointData;
RaceCheckpointData CCheckPoint::m_raceCheckpointData;

void CCheckPoint::Initialise()
{
    memset(&m_checkpointData, 0, sizeof(m_checkpointData));
    memset(&m_raceCheckpointData, 0, sizeof(m_raceCheckpointData));
}

void CCheckPoint::Create(CVector *pos, CVector *extent)
{
    memcpy(&m_checkpointData.vecCheckpointPos, pos, sizeof(CVector));
    memcpy(&m_checkpointData.vecCheckpointExtent, extent, sizeof(CVector));

    if (CCheckPoint::m_checkpointData.dwCheckpointSphere && CCheckPoint::m_checkpointData.dwCheckpointMarker)
    {
        CMarkerIcon::Destroy(m_checkpointData.dwCheckpointMarker);
        CScripting::ScriptCommand(&destroy_sphere, m_checkpointData.dwCheckpointSphere);
        CCheckPoint::m_checkpointData.dwCheckpointMarker = 0;
        CCheckPoint::m_checkpointData.dwCheckpointSphere = 0;
        CGui::GetGPSNavigator()->SetPosition(0.0f, 0.0f, 0.0f);
        CGui::GetGPSNavigator()->SetVisible(false);
    }

    CCheckPoint::m_checkpointData.dwCheckpointMarker = CMarkerIcon::Create(0, CCheckPoint::m_checkpointData.vecCheckpointPos.x, CCheckPoint::m_checkpointData.vecCheckpointPos.y, CCheckPoint::m_checkpointData.vecCheckpointPos.z, 1005, 0);
    CScripting::ScriptCommand(&create_sphere, CCheckPoint::m_checkpointData.vecCheckpointPos.x, CCheckPoint::m_checkpointData.vecCheckpointPos.y, CCheckPoint::m_checkpointData.vecCheckpointPos.z, CCheckPoint::m_checkpointData.vecCheckpointExtent.x, &m_checkpointData.dwCheckpointSphere);
    CGui::GetGPSNavigator()->SetPosition(CCheckPoint::m_checkpointData.vecCheckpointPos.x, CCheckPoint::m_checkpointData.vecCheckpointPos.y, CCheckPoint::m_checkpointData.vecCheckpointPos.z);
    CGui::GetGPSNavigator()->SetVisible(true);
}

void CCheckPoint::CreateRace(int8_t byteType, CVector *pos, CVector *next, float fSize)
{
    memcpy(&m_raceCheckpointData.vecRaceCheckpointPos, pos, sizeof(CVector));
    memcpy(&m_raceCheckpointData.vecRaceCheckpointNext, next,sizeof(CVector));

    CCheckPoint::m_raceCheckpointData.fRaceCheckpointSize = fSize;
    CCheckPoint::m_raceCheckpointData.byteRaceType = byteType;

    if (CCheckPoint::m_raceCheckpointData.dwRaceCheckpointMarker)
    {
        CMarkerIcon::Destroy(m_raceCheckpointData.dwRaceCheckpointMarker);
        CCheckPoint::m_raceCheckpointData.dwRaceCheckpointMarker = 0;

        CCheckPoint::m_raceCheckpointData.dwRaceCheckpointMarker = CMarkerIcon::Create(0, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.x, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.y, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.z, 1005, 0);
    }

    CCheckPoint::DisableRace();

    CScripting::ScriptCommand(&create_racing_checkpoint, (int)m_raceCheckpointData.byteRaceType, m_raceCheckpointData.vecRaceCheckpointPos.x, m_raceCheckpointData.vecRaceCheckpointPos.y, m_raceCheckpointData.vecRaceCheckpointPos.z, m_raceCheckpointData.vecRaceCheckpointNext.x, m_raceCheckpointData.vecRaceCheckpointNext.y, m_raceCheckpointData.vecRaceCheckpointNext.z, m_raceCheckpointData.fRaceCheckpointSize, &m_raceCheckpointData.dwRaceCheckpointHandle);
    CGui::GetGPSNavigator()->SetPosition(CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.x, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.y, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.z);
    CGui::GetGPSNavigator()->SetVisible(true);
    CCheckPoint::m_raceCheckpointData.bRaceCheckpointsEnabled = true;
}

void CCheckPoint::Process()
{
    if (CCheckPoint::m_checkpointData.bCheckpointsEnabled)
    {
        if (CGame::GetPlayer())
        {
            CScripting::ScriptCommand(&is_actor_near_point_3d, CGame::GetPlayer()->m_dwGTAId, m_checkpointData.vecCheckpointPos.x, m_checkpointData.vecCheckpointPos.y, m_checkpointData.vecCheckpointPos.z, m_checkpointData.vecCheckpointExtent.x, m_checkpointData.vecCheckpointExtent.y, m_checkpointData.vecCheckpointExtent.z, 1);

            if (!CCheckPoint::m_checkpointData.dwCheckpointSphere)
            {
                CCheckPoint::m_checkpointData.dwCheckpointMarker = CMarkerIcon::Create(0, CCheckPoint::m_checkpointData.vecCheckpointPos.x, CCheckPoint::m_checkpointData.vecCheckpointPos.y, CCheckPoint::m_checkpointData.vecCheckpointPos.z, 1005, 0);
                CScripting::ScriptCommand(&create_sphere, CCheckPoint::m_checkpointData.vecCheckpointPos.x, CCheckPoint::m_checkpointData.vecCheckpointPos.y, CCheckPoint::m_checkpointData.vecCheckpointPos.z, CCheckPoint::m_checkpointData.vecCheckpointExtent.x, &m_checkpointData.dwCheckpointSphere);
                CGui::GetGPSNavigator()->SetPosition(CCheckPoint::m_checkpointData.vecCheckpointPos.x, CCheckPoint::m_checkpointData.vecCheckpointPos.y, CCheckPoint::m_checkpointData.vecCheckpointPos.z);
                CGui::GetGPSNavigator()->SetVisible(true);
            }
        }
    }
    else if (CCheckPoint::m_checkpointData.dwCheckpointSphere && CCheckPoint::m_checkpointData.dwCheckpointMarker)
    {
        CMarkerIcon::Destroy(m_checkpointData.dwCheckpointMarker);
        CScripting::ScriptCommand(&destroy_sphere, m_checkpointData.dwCheckpointSphere);
        CCheckPoint::m_checkpointData.dwCheckpointMarker = 0;
        CCheckPoint::m_checkpointData.dwCheckpointSphere = 0;
        CGui::GetGPSNavigator()->SetPosition(0.0f, 0.0f, 0.0f);
        CGui::GetGPSNavigator()->SetVisible(false);
    }

    if (CCheckPoint::m_raceCheckpointData.bRaceCheckpointsEnabled)
    {
        if (CGame::GetPlayer())
        {
            if (!CCheckPoint::m_raceCheckpointData.dwRaceCheckpointMarker)
            {
                CCheckPoint::m_raceCheckpointData.dwRaceCheckpointMarker = CMarkerIcon::Create(0, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.x, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.y, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.z, 1005, 0);
                CGui::GetGPSNavigator()->SetPosition(CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.x, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.y, CCheckPoint::m_raceCheckpointData.vecRaceCheckpointPos.z);
                CGui::GetGPSNavigator()->SetVisible(true);
            }
        }
    }
    else if (CCheckPoint::m_raceCheckpointData.dwRaceCheckpointMarker)
    {
        CMarkerIcon::Destroy(m_raceCheckpointData.dwRaceCheckpointMarker);
        CCheckPoint::DisableRace();
        CCheckPoint::m_raceCheckpointData.dwRaceCheckpointMarker = 0;
        CGui::GetGPSNavigator()->SetPosition(0.0f, 0.0f, 0.0f);
        CGui::GetGPSNavigator()->SetVisible(false);
    }
}

void CCheckPoint::DisableRace()
{
    if (CCheckPoint::m_raceCheckpointData.dwRaceCheckpointHandle)
    {
        CScripting::ScriptCommand(&destroy_racing_checkpoint, m_raceCheckpointData.dwRaceCheckpointHandle);
        CCheckPoint::m_raceCheckpointData.dwRaceCheckpointHandle = 0;
        CGui::GetGPSNavigator()->SetPosition(0.0f, 0.0f, 0.0f);
        CGui::GetGPSNavigator()->SetVisible(false);
    }

    CCheckPoint::m_raceCheckpointData.bRaceCheckpointsEnabled = false;
}

void CCheckPoint::ToggleCheckpoints(bool bEnabled)
{
    CCheckPoint::m_checkpointData.bCheckpointsEnabled = bEnabled;
}

void CCheckPoint::ToggleRaceCheckpoints(bool bEnabled)
{
    CCheckPoint::m_raceCheckpointData.bRaceCheckpointsEnabled = bEnabled;
}