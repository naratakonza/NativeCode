#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct RwTexture;

class CTxdManager
{
public:
    static void DestroyTexture(RwTexture* texture);
    static uintptr_t LoadTextureFromDB(const char* dbname, const char* texture);
    static uintptr_t LoadTexture(const char* texname, bool bSkipBadCheck = false);
    static RwTexture* FindTexture(const char* szTexture);
    static RwTexture* LoadTextureFromTxd(const char *szTxdName, const char *szTexture);
    static bool LoadTxdSlotIfNeeded(const char *szTxdName);
    static bool IsBadTexture(const char* texture);
    static void ParseDatabaseRedirects(const std::string& dbname);
    static RwTexture* LoadTextureFromFile(const char* filepath);

public:
    static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_mapDatabaseRedirects;

private:
    static std::vector<std::string> badTextures;
    static std::unordered_map<std::string, bool> m_mapParsedDatabases;
};