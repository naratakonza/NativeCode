#pragma once

#include "../CEntity.h"
// #include "../Vehicle/CVehicle.h"
#include "../../../utils/Quat/CQuaternion.h"
#include "../../Core/CMatrix.h"
#include "../../Enums/eInvalidId.h"
#include "../../Enums/eSpecialObject.h"
#include "../../Enums/eMaterialType.h"
#include "../../Enums/eMaterialSize.h"
#include "../../Enums/eMaterialTextAlign.h"

struct RpClumpAtomicsData;

struct SavedMaterial
{
    RpMaterial* material;
    RwRGBA color;
};

struct SavedGeometry
{
    RpGeometry* geometry;
    uint32_t flags;
    bool allocatedPrelit;
    RwRGBA* originalPrelitPtr;
    std::vector<RwRGBA> originalPrelitVal;
    std::vector<SavedMaterial> savedMaterials;
};

struct ModifyParams
{
    std::vector<SavedGeometry>& saved;
    bool bIsObjectModel;
};

#define MAX_MATERIALS_PER_MODEL 16

typedef struct _MATERIAL_OBJECT {
    int             iType;
    bool            bTextureWasCreated;
    bool            bMaterialWasCreated;
    bool            bMaterialColorWasCreated;

    typedef struct _MATERIAL_TEXT {
        uint8_t     byteMaterialSize;
        char        szFontName[32];
        uint8_t     byteFontSize;
        uint8_t     byteFontBold;
        uint32_t    dwFontColor;
        uint32_t    dwBackgroundColor;
        uint8_t     byteAlign;
        char        szText[2048];
    } MATERIAL_TEXT;

    MATERIAL_TEXT   TextInfo;
    RwTexture*      pMaterialTexture;
    uint32_t        dwMaterialColor;
    RwTexture*      originalMaterialTexture;
    RwRGBA          originalMaterialColor;
    uint32_t        originalGeometryFlags;
} MATERIAL_OBJECT;

class CObject : public CEntity
{
public:
    CMatrix         m_matTarget;
    uint8_t         m_byteMoving;
    float           m_fMoveSpeed;
    bool            m_bNeedRotate;
    CQuaternion     m_quatStart;
    CQuaternion     m_quatTarget;
    uint32_t        m_dwMoveTick;

    uint16_t        m_iModel;
    uint16_t        m_attachedToVehicle;
    uint16_t        m_attachedToObject;
    CVector         m_vecAttachOffset;
    CVector         m_vecAttachRotation;
    uint8_t         m_byteSyncRotation;
    bool            m_bDontCollideWithCamera;
    float           m_fDrawDistance;
    float           m_fDistanceToCamera;
    float           m_fRadius;
    int             m_iAlpha;

    CVector         m_vecRotation;
    float           m_fDistanceToTargetPoint;

    CVector         m_vecSpawnPos;

    MATERIAL_OBJECT m_Material[MAX_MATERIALS_PER_MODEL];

    bool            m_bHasMaterial;
    bool            m_bMaterialTextureChanged;
    bool            m_bMaterialColorChanged;
    bool            m_bCanUpdateMaterial;
    RwObject*       m_pLastMaterialRwObject;

    bool            m_bCanChangeAccessoryMaterial;
    bool            m_bAccessoryMaterialChanged;

    RwTexture*      m_bLankTexture;
    bool            m_bDrawLast;
    uint32_t        m_dwStreamingTimer;

public:
    CObject(int iModel, float fPosX, float fPosY, float fPosZ, CVector vecRot, float fDrawDistance, bool bAccessory = false);
    ~CObject();

    static void SetDirectModelCreation(bool enabled);
    void DestroyMaterials();

    void SetMaterial(int iModel, uint8_t byteMaterialIndex, char* txdname, char* texturename, uint32_t dwColor);
    void SetMaterialText(uint8_t byteMaterialIndex, uint8_t byteMaterialSize, const char* szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char* szText);
    void CreateMaterialFontTexture();
    bool ConvertMaterialTextSize(int iMatSize, int *iSizeX, int *iSizeY);

    void ProcessMaterialUpdates();
    void HandleCustomMaterialUpdate();
    void ProcessMaterialRestore();
    void SetupAccessoryLighting();
    void ResetAccessoryLighting();

    void Process(float fElapsedTime);
    void ProcessStreaming();
    void Draw();

    void ApplyMoveSpeed();
    float DistanceRemaining(CMatrix *matPos, CMatrix *m_matPositionTarget);
    float RotationRemaining(CVector matPos, CVector m_vecRot);

    void MoveTo(float x, float y, float z, float speed, float rX, float rY, float rZ);
    void InstantRotate(float x, float y, float z);
    void RotateMatrix(float x, float y, float z);
    void StopMoving();
    void ProcessAttachedObject();
    // void SetAttachedToVehicle(uint16_t vehicleId, CVector vecOrgAttachOffset, CVector vecOrgAttachRotation);
    void SetAttachedToObject(uint16_t objectId, CVector vecOrgAttachOffset, CVector vecOrgAttachRotation, uint8_t byteSyncRotation);
    // void AttachToVehicle(CVehicle *pVehicle);
    void AttachToObject(CObject *pObject);
    uint32_t IsObjectAttached(uint32_t iID);

    void SetAlpha(int nAlpha);
    int GetAlpha();

    bool AttachedToMovingEntity();
    void Render();
    void SetFlags();
    void UpdateRenderPosition();
    static RpAtomic* FixObjectLightingCB(RpAtomic* rpAtomic, void* data);
    static RpAtomic* ModifyGeometryCB(RpAtomic* rpAtomic, void* data);
    static void ModifyGeometry(RpGeometry* geometry, std::vector<SavedGeometry>& saved, bool bIsObjectModel);
    static void ProcessObjectGeometries(uintptr_t pRwObject, std::vector<SavedGeometry>& saved, bool bIsObjectModel);
    static void RestoreObjectGeometries(const std::vector<SavedGeometry>& saved);
};