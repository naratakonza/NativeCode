#pragma once

class CObject;
struct CEntityGta;

class CGtaObjectCache
{
private:
    static std::unordered_map<CEntityGta*, CObject*> m_ObjectCache;
    static std::unordered_map<CEntityGta*, CObject*> m_AccessoryCache;
    static bool m_bNeedsUpdate;
    static uint32_t m_dwLastUpdateTick;

public:
    static void Update();

    static void SetDirty() {
        CGtaObjectCache::m_bNeedsUpdate = true;
    }

    static void Remove(CObject* pObject);
    static CObject* FindObject(CEntityGta* pEntity);
    static CObject* FindAccessory(CEntityGta* pEntity);
};