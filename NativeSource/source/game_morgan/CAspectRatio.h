#pragma once

#include "Enums/eAspectRatioType.h"

struct RwCamera;
struct RwRect;

class CAspectRatio
{
public:
    static float ms_fAspectRatioSize;

public:
    static void Initialise();
    static void SetSize(eAspectRatioType type);
    static void CameraSize(RwCamera* camera, RwRect* rect, float viewWindow, float aspectRatio);

    static float GetSize()
    {
        return CAspectRatio::ms_fAspectRatioSize;
    }
};