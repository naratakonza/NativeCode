#include "../../Main.h"
#include "CTxdManager.h"
#include "CTxdStore.h"
#include "CSprite2d.h"
#include "TextureDatabaseRuntime.h"
#include "../utils/Storage/CStorage.h"

#include <algorithm>
#include <cctype>
#include <unistd.h>

std::vector<std::string> CTxdManager::badTextures = { "none", "", " " };
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> CTxdManager::m_mapDatabaseRedirects;
std::unordered_map<std::string, bool> CTxdManager::m_mapParsedDatabases;

void CTxdManager::DestroyTexture(RwTexture* texture)
{
    if (texture && RwTextureDestroy)
    {
        RwTextureDestroy(texture);
    }
}

uintptr_t CTxdManager::LoadTextureFromDB(const char* dbname, const char* texture)
{
    if (CTxdManager::IsBadTexture(texture)) {
        return 0;
    }

    uintptr_t db_handle = TextureDatabaseRuntime::GetDatabase(dbname);

    if (!db_handle) {
        CNecessary::Log(OBFUSCATE("Error: Database not found! (%s)"), dbname);
        return 0;
    }

    TextureDatabaseRuntime::Register(db_handle);
    uintptr_t tex = TextureDatabaseRuntime::GetTextureByName(texture);

    if (!tex) {
        CNecessary::Log(OBFUSCATE("Error: Texture (%s) not found in database (%s)"), texture, dbname);
        TextureDatabaseRuntime::Unregister(db_handle);
        return 0;
    }

    TextureDatabaseRuntime::Unregister(db_handle);

    //CNecessary::Log(OBFUSCATE("Successfully: Loaded Texture (%s) from database (%s)"), texture, dbname);

    return tex;
}

uintptr_t CTxdManager::LoadTexture(const char* texname, bool bSkipBadCheck)
{
    if (!texname || (!bSkipBadCheck && CTxdManager::IsBadTexture(texname))) {
        return 0;
    }

    static const char* texdb[] = { OBFUSCATE("samp"), OBFUSCATE("mobile"), OBFUSCATE("txd"), OBFUSCATE("gta3"), OBFUSCATE("gta_int") };

    for (const auto& dbname : texdb)
    {
        uintptr_t texture = CTxdManager::LoadTextureFromDB(dbname, texname);
        if (texture)
        {
            return texture;
        }
    }

    CTxdManager::badTextures.emplace_back(texname);
    return 0;
}

RwTexture* CTxdManager::FindTexture(const char* szTexture)
{
    RwTexture* tex = reinterpret_cast<RwTexture*>(CTxdManager::LoadTexture(szTexture));

    if (!tex && szTexture)
    {
        std::string str = szTexture;
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
        tex = reinterpret_cast<RwTexture*>(CTxdManager::LoadTexture(str.c_str()));
    }

    return tex;
}

RwTexture *CTxdManager::LoadTextureFromTxd(const char *szTxdName, const char *szTexture)
{
    RwTexture *pTexture = 0;
    if (szTxdName && szTexture)
    {
        int iTxdSlot = CTxdStore::FindTxdSlot(szTxdName);
        if (iTxdSlot != -1)
        {
            CTxdStore::PushCurrentTxd();
            CTxdStore::SetCurrentTxd(iTxdSlot);

            pTexture = CSprite2d::SetTexture(szTexture);
            if (!pTexture)
            {
                pTexture = CTxdManager::FindTexture(szTexture);
            }

            CTxdStore::PopCurrentTxd();
        }
    }

    return pTexture;
}

bool CTxdManager::LoadTxdSlotIfNeeded(const char *szTxdName)
{
    if (!szTxdName) {
        return false;
    }

    int iTxdSlot = CTxdStore::FindTxdSlot(szTxdName);
    if (iTxdSlot != -1) {
        return true;
    }

    if (CTxdStore::FindTxdSlot(szTxdName) == -1)
    {
        constexpr std::size_t txdNameCapacity = 20;
        if (strnlen(szTxdName, txdNameCapacity) == txdNameCapacity) {
            return false;
        }

        int iTxdSlot = CTxdStore::AddTxdSlot(szTxdName, OBFUSCATE("txd"));
        if (iTxdSlot == -1) {
            return false;
        }

        CTxdStore::AddRef(iTxdSlot);
        return true;
    }

    return false;
}

bool CTxdManager::IsBadTexture(const char* texture)
{
    if (!texture) {
        return true;
    }

    std::string strTexture(texture);
    return std::find(badTextures.begin(), badTextures.end(), strTexture) != badTextures.end();
}

void CTxdManager::ParseDatabaseRedirects(const std::string& dbname)
{
    if (CTxdManager::m_mapParsedDatabases[dbname]) {
        return;
    }

    CTxdManager::m_mapParsedDatabases[dbname] = true;

    auto openDatabase = [](const std::string& name) -> morgan::fileio::File
    {
        using namespace morgan::fileio;
        const std::string path = std::string(OBFUSCATE("texdb/")) + name + "/" + name + ".txt";
        const auto asset = ResolveGameAssetPath(path);
        if (asset.disposition != Disposition::Redirect || asset.group >= storage::GroupCount ||
            std::string_view(storage::GroupNames[asset.group]) != "texdb") {
            return {};
        }

        auto active = CStorage::OpenActiveResource(nullptr, asset);
        if (active.file || active.error != std::errc::no_such_file_or_directory) {
            return std::move(active.file);
        }

        const auto bundleRoot = CStorage::GameBundleResourcePath();
        if (bundleRoot.empty() || CStorage::IsAssetFreeBundle()) {
            return {};
        }

        const std::string bundlePath = std::string(bundleRoot) + "/" + std::string(asset.Path());
        return File(std::fopen(bundlePath.c_str(), OBFUSCATE("r")));
    };

    auto file = openDatabase(dbname);
    if (!file)
    {
        std::string lowerDbname = dbname;
        std::transform(lowerDbname.begin(), lowerDbname.end(), lowerDbname.begin(), [](unsigned char c) { return std::tolower(c); });
        if (lowerDbname != dbname) {
            file = openDatabase(lowerDbname);
        }
        
        if (!file) {
            return;
        }
    }

    char line[512];
    while (fgets(line, sizeof(line), file.get()))
    {
        std::string strLine(line);
        size_t affPos = strLine.find(static_cast<const char*>(OBFUSCATE("affiliate=")));
        if (affPos == std::string::npos) {
            continue;
        }

        size_t firstQuote = strLine.find('"');
        if (firstQuote == std::string::npos) {
            continue;
        }

        size_t secondQuote = strLine.find('"', firstQuote + 1);
        if (secondQuote == std::string::npos) {
            continue;
        }

        std::string srcName = strLine.substr(firstQuote + 1, secondQuote - firstQuote - 1);

        std::string targetName = strLine.substr(affPos + 10);
        if (!targetName.empty() && targetName.front() == '"')
        {
            targetName.erase(targetName.begin());
        }

        size_t spacePos = targetName.find(' ');
        if (spacePos != std::string::npos)
        {
            targetName = targetName.substr(0, spacePos);
        }

        while (!targetName.empty() && (targetName.back() == '"' || targetName.back() == '\r' || targetName.back() == '\n' || targetName.back() == ' '))
        {
            targetName.pop_back();
        }

        if (!srcName.empty() && !targetName.empty())
        {
            std::string srcNameLower = srcName;
            std::string targetNameLower = targetName;
            std::transform(srcNameLower.begin(), srcNameLower.end(), srcNameLower.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(targetNameLower.begin(), targetNameLower.end(), targetNameLower.begin(), [](unsigned char c) { return std::tolower(c); });

            CTxdManager::m_mapDatabaseRedirects[dbname][srcNameLower] = targetNameLower;
        }
    }
}

RwTexture* CTxdManager::LoadTextureFromFile(const char* filepath)
{
    if (!filepath) {
        return nullptr;
    }

    if (access(filepath, R_OK) != 0) {
        CNecessary::Log(OBFUSCATE("LoadTextureFromFile: File not found: %s"), filepath);
        return nullptr;
    }

    if (!RwImageDestroy || !RwImageFindRasterFormat || !RwRasterCreate || !RwRasterSetFromImage || !RwRasterDestroy || !RwTextureCreate) {
        return nullptr;
    }

    RwImage* image = nullptr;
    std::string pathStr(filepath);

    if (pathStr.find(OBFUSCATE(".png")) != std::string::npos || pathStr.find(OBFUSCATE(".PNG")) != std::string::npos)
    {
        if (RtPNGImageRead)
        {
            image = RtPNGImageRead(filepath);
        }
    }
    else
    {
        if (RwImageRead)
        {
            image = RwImageRead(filepath);
        }
    }

    if (!image) {
        CNecessary::Log(OBFUSCATE("LoadTextureFromFile: Failed to read image: %s"), filepath);
        return nullptr;
    }

    if (!image->cpPixels) {
        CNecessary::Log(OBFUSCATE("LoadTextureFromFile: Image pixels are null: %s"), filepath);
        RwImageDestroy(image);
        return nullptr;
    }

    if (image->width <= 0 || image->height <= 0) {
        CNecessary::Log(OBFUSCATE("LoadTextureFromFile: Invalid image dimensions: %dx%d"), image->width, image->height);
        RwImageDestroy(image);
        return nullptr;
    }

    RwInt32 w = 0, h = 0, d = 0, flags = 0;
    if (!RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, &w, &h, &d, &flags)) {
        RwImageDestroy(image);
        return nullptr;
    }

    RwRaster* raster = RwRasterCreate(w, h, d, flags);
    if (!raster) {
        CNecessary::Log(OBFUSCATE("LoadTextureFromFile: Failed to create raster"));
        RwImageDestroy(image);
        return nullptr;
    }

    raster = RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);

    if (!raster) {
        CNecessary::Log(OBFUSCATE("LoadTextureFromFile: Failed to set raster from image"));
        return nullptr;
    }

    RwTexture* texture = RwTextureCreate(raster);
    if (!texture) {
        CNecessary::Log(OBFUSCATE("LoadTextureFromFile: Failed to create texture"));
        RwRasterDestroy(raster);
        return nullptr;
    }

    texture->refCount = 1;
    return texture;
}