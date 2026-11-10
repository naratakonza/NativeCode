#include "CUtil.h"

#include <unordered_map>

std::string CUtil::ReplaceKey(const std::string& input)
{
    static const std::unordered_map<std::string, std::string> keyMap = {
        {"~k~~PED_ANSWER_PHONE~", "TAB"},
        {"~k~~VEHICLE_FIREWEAPON_ALT~", "ALT GR / LCTRL / NUM0"},
        {"~k~~PED_DUCK~", "C"},
        {"~k~~VEHICLE_HORN~", "H / CAPSLOCK"},
        {"~k~~PED_FIREWEAPON~", "LCTRL / LMB"},
        {"~k~~VEHICLE_FIREWEAPON~", "LALT"},
        {"~k~~PED_SPRINT~", "SPC"},
        {"~k~~VEHICLE_ACCELERATE~", "W"},
        {"~k~~VEHICLE_ENTER_EXIT~", "ENTER"},
        {"~k~~PED_JUMPING~", "LSHIFT"},
        {"~k~~VEHICLE_BRAKE~", "S"},
        {"~k~~VEHICLE_LOOKRIGHT~", "E"},
        {"~k~~PED_LOCK_TARGET~", "RMB"},
        {"~k~~VEHICLE_HANDBRAKE~", "SPC"},
        {"~k~~VEHICLE_LOOKLEFT~", "Q"},
        {"~k~~PED_LOOKBEHIND~", "NUM1 / MMB"},
        {"~k~~VEHICLE_LOOKBEHIND~", "2"},
        {"~k~~TOGGLE_SUBMISSIONS~", "2"},
        {"~k~~SNEAK_ABOUT~", "LALT"},
        {"~k~~VEHICLE_TURRETUP~", "NUM8"},
        {"~k~~VEHICLE_TURRETDOWN~", "NUM2"},
        {"~k~~VEHICLE_TURRETLEFT~", "NUM4"},
        {"~k~~VEHICLE_TURRETRIGHT~", "NUM6"},
        {"~k~~CONVERSATION_YES~", "Y"},
        {"~k~~CONVERSATION_NO~", "N"},
        {"~k~~GROUP_CONTROL_BWD~", "H"},
        {"~k~~GROUP_CONTROL_FWD~", "G"},
        {"~k~~GO_FORWARD~", "UP"},
        {"~k~~VEHICLE_STEERUP~", "UP"},
        {"~k~~GO_BACK~", "DOWN"},
        {"~k~~VEHICLE_STEERDOWN~", "DOWN"},
        {"~k~~GO_LEFT~", "LEFT"},
        {"~k~~VEHICLE_STEERLEFT~", "LEFT"},
        {"~k~~GO_RIGHT~", "RIGHT"},
        {"~k~~VEHICLE_STEERRIGHT~", "RIGHT"}
    };

    std::string output = input;

    for (const auto& pair : keyMap)
    {
        std::string::size_type pos = 0;
        while ((pos = output.find(pair.first, pos)) != std::string::npos)
        {
            output.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }

    return output;
}
