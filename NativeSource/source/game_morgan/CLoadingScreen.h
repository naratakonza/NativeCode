#pragma once

class CLoadingScreen
{
public:
    static void SetInterface(bool state);
    static void UpdatePercent(int intMult);
    static void UpdateList(const char* FileName);
};