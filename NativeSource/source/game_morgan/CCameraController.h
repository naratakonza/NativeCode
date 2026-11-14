#pragma once

#include "Enums/eCameraMode.h"
#include "Enums/eCamMode.h"

class CCameraController
{
public:
    static bool m_bAimed;
    static float fVerticalAngle;
    static float fHorizontalAngle;

public:
    static void Initialise();
    static bool IsCameraAllAiming(bool reset, bool zoomOnly = false);
    static void AimForTheCamera();
    static void SetAimAngles(float verticalAngle, float horizontalAngle);
};