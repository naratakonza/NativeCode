#pragma once

struct RwTexDictionary;

class CTxdStore
{
public:
    static void PushCurrentTxd();
    static void PopCurrentTxd();
    static int FindTxdSlot(const char* name);
    static void SetCurrentTxd(int index);
    static int AddTxdSlot(const char* szTxdName, const char* szTexdb);
    static void AddRef(int iTxdSlot);
    static RwTexDictionary* GetTxdParent(RwTexDictionary* pTxd);
};