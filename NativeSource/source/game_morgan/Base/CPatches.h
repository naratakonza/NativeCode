#pragma once

#include "../Enums/ePatchType.h"

class CPatches
{
public:
    static void SetPatches(ePatchType type);

private:
    static void InstallLoadFilePatches();
    static void InstallLevelZPatches();
    static void InstallSavePatches();
    static void InstallSettingGamePatches();
    static void InstallTemporaryFrameratePatches();
    static void InstallMainPatches();
    static void InstallGameProcessPatches();
    static void InstallGraphicsOptimizationPatches();
    static void InstallPadPatches();
    static void InstallATZPatches();
    static void InstallRadarPatches();
    static void InstallDisableAutoAimPatches();
    static void InstallCameraPatches();
};
