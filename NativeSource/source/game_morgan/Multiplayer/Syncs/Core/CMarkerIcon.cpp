#include "../../../../../Main.h"
#include "CMarkerIcon.h"
#include "../../../Scripts/CScripting.h"

uint32_t CMarkerIcon::Create(uint8_t iMarkerType, float fX, float fY, float fZ, uint32_t iColor, uint8_t iStyle)
{
    uint32_t dwMarkerID = 0;

    switch (iStyle)
    {
        case 0:
        {
            CScripting::ScriptCommand(&create_radar_marker_without_sphere, fX, fY, fZ, iMarkerType, &dwMarkerID);
            break;
        }

        case 1:
        {
            CScripting::ScriptCommand(&create_marker_icon, fX, fY, fZ, iMarkerType, &dwMarkerID);
            break;
        }

        case 2:
        {
            CScripting::ScriptCommand(&create_radar_marker_icon, fX, fY, fZ, iMarkerType, &dwMarkerID);
            break;
        }

        case 3:
        {
            CScripting::ScriptCommand(&create_icon_marker_sphere, fX, fY, fZ, iMarkerType, &dwMarkerID);
            break;
        }
    }

    if (dwMarkerID == UINT32_MAX) {
        return 0;
    }

    if (dwMarkerID && iMarkerType == 0)
    {
        CScripting::ScriptCommand(&set_marker_color, static_cast<int>(dwMarkerID), static_cast<int>(iColor));

        if (iColor >= 1004)
        {
            CScripting::ScriptCommand(&show_on_radar, static_cast<int>(dwMarkerID), 3);
        }
        else
        {
            CScripting::ScriptCommand(&show_on_radar, static_cast<int>(dwMarkerID), 2);
        }
    }

    return dwMarkerID;
}

void CMarkerIcon::Destroy(uint32_t dwMarkerId)
{
    CScripting::ScriptCommand(&disable_marker, static_cast<int>(dwMarkerId));
}