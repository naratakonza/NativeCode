#pragma once

#include "../RenderWare/rtquat.h"

class BoneNode_c
{
public:
    static void EulerToQuat(RwV3d* angles, RtQuat* orientation);
};