#include "../../Main.h"
#include "CTheZones.h"
#include "Scripts/CScripting.h"

void CTheZones::EnableZoneNames(bool bEnable)
{
	CScripting::ScriptCommand(&enable_zone_names, bEnable);
}