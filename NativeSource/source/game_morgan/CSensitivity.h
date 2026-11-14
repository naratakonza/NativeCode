#pragma once

class CSensitivity
{
public:
    static float& ms_fOnfootTouch;
    static float ms_fAimTouch;
    static float ms_fIncarTouch;

public:
    static void SetOnfootTouch(float value);
    static void SetAimTouch(float value);
    static float GetAimTouch();
    static void SetIncarTouch(float value);
    static float GetIncarTouch();
};
