#pragma once

#include "../../../Enums/eInvalidId.h"
#include "../../../Entity/Object/CObject.h"

class CObjectPool
{
private:
    std::unordered_map<uint16_t, CObject*> m_pObjects;
    std::unordered_map<uint16_t, bool> m_bObjectsSlot;

public:
    CObjectPool();
    ~CObjectPool();
    void Process();
    void Draw();
    void ProcessMaterialText();
    void DeleteAll();

    CObject* GetObject(uint16_t id) {
        auto it = CObjectPool::m_pObjects.find(id);
        if (it != CObjectPool::m_pObjects.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool GetObjectInSlot(uint16_t id) {
        auto it = CObjectPool::m_bObjectsSlot.find(id);
        if (it != CObjectPool::m_bObjectsSlot.end()) {
            return it->second;
        }

        return false;
    }

    void Delete(uint16_t ObjectID);
    bool Add(uint16_t ObjectID, int modelID, CVector vecPos, CVector vecRot, float fDrawDistance);
    void EjectCameraFromObject(uint16_t ObjectID);
    CObject* GetObjectFromGtaPtr(CEntityGta* pGtaObject) const;
    uint16_t FindIDFromGtaPtr(CEntityGta *pGtaObject) const;
    uint16_t FindIDFromCameraRay(float maxDistance, CVector* cameraPos, CVector* rayEnd);
    bool ObjectCollisionProcess(uintptr_t CColSphere1, uintptr_t CColSphere2);
    bool IsValidPointer(uint16_t ObjectID, CObject* pObject, bool m_bDebug = true);
};