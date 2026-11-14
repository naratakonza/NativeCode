#pragma once

class CdStreamInfo
{
public:
    static bool IsAvailable();
    static int CdStreamOpen(const char* fileName, bool isPlayerIMG);
};
