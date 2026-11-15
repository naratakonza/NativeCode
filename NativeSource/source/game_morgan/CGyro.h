#pragma once

#include "Enums/eGyroMode.h"

class CGyro
{
private:
    static float m_fGyroX;
    static float m_fGyroY;
    static float m_fGyroZ;
    static bool m_bEnable;
    static float ms_fAimSens;
    static float ms_fOnfootSens;
    static float ms_fIncarSens;
    static eGyroMode m_ntype;

public:
    static void SetGyroMode(eGyroMode mode);
    static eGyroMode GetGyroMode();
    static void SetAimSensitivity(float value);
    static float GetAimSensitivity();
    static void SetOnfootSensitivity(float value);
    static float GetOnfootSensitivity();
    static void SetIncarSensitivity(float value);
    static float GetIncarSensitivity();
    static void SetGyroMoveSpeed(float x, float y, float z);
    static float GetGyroX();
    static float GetGyroY();
    static float GetGyroZ();
    static double GetElapsedTime();
    static void Enable(bool enable);
    static bool IsGyro();
};