#include "../../../../Main.h"
#include "CModelSnapshot.h"
#include "../../CCommon.h"
#include "../../CStreaming.h"

#include <mach-o/dyld.h>
#include <new>

static_assert(offsetof(RwCamera, frameBuffer) == 0x80);
static_assert(offsetof(RwCamera, zBuffer) == 0x88);

CModelSnapshot::CModelSnapshot()
{
    CNecessary::Log(OBFUSCATE("CModelSnapshot Initialise"));

    m_camera = 0;
    m_light = 0;
    m_frame = 0;
    m_zBuffer = 0;

    SetUpScene();
}

CModelSnapshot::~CModelSnapshot()
{
    if (m_camera)
    {
        CCommon::rpWorldRemoveCamera(m_camera);
        m_camera->frameBuffer = nullptr;
        m_camera->zBuffer = nullptr;
        RwCameraDestroy(m_camera);
    }

    if (m_frame)
	{
        RwFrameDestroy(m_frame);
    }

    if (m_zBuffer)
	{
        RwRasterDestroy(m_zBuffer);
    }

    if (m_light)
	{
        RpLightDestroy(m_light);
    }
}

void CModelSnapshot::SetUpScene()
{
    if (!RwCameraCreate || !RwCameraDestroy || !RwFrameCreate || !RwFrameDestroy || !RwFrameTranslateAMZ || !RwFrameRotateAMZ || !RwRasterCreate || !RwRasterDestroy || !RwObjectHasFrameSetFrame || !RwCameraSetFarClipPlane || !RwCameraSetNearClipPlane || !RwCameraSetViewWindow || !RwCameraSetProjection || !RpLightCreate || !RpLightSetColor || !RpLightDestroy) {
        return;
    }

    m_light = RpLightCreate(rpLIGHTAMBIENT);
    if (!m_light) {
        return;
    }

    RwRGBAReal rwColor = {1.0f, 1.0f, 1.0f, 1.0f};
    RpLightSetColor(m_light, &rwColor);
    m_light->object.object.flags |= rpLIGHTLIGHTATOMICS;

    m_camera = RwCameraCreate();
    m_frame = RwFrameCreate();
    if (!m_camera || !m_frame) {
        return;
    }

    RwV3d v = {0.0f, 0.0f, 50.0f};
    RwFrameTranslateAMZ(m_frame, &v, rwCOMBINEPRECONCAT);
    v = {1.0f, 0.0f, 0.0f};
    RwFrameRotateAMZ(m_frame, &v, 90.0f, rwCOMBINEPRECONCAT);

    m_zBuffer = RwRasterCreate(256, 256, 0, rwRASTERTYPEZBUFFER);
    if (!m_zBuffer) {
        return;
    }

    m_camera->zBuffer = m_zBuffer;
    RwObjectHasFrameSetFrame(m_camera, m_frame);
    RwCameraSetFarClipPlane(m_camera, 300.0f);
    RwCameraSetNearClipPlane(m_camera, 0.01f);
    const RwV2d view = {0.5f, 0.5f};
    RwCameraSetViewWindow(m_camera, view);
    RwCameraSetProjection(m_camera, rwPERSPECTIVE);

    CCommon::rpWorldAddCamera(m_camera);
}

RwTexture* CModelSnapshot::CreatePedSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom)
{
    RwRaster *raster = RwRasterCreate(256, 256, 0, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
    if (!raster) {
        return 0;
    }

    RwTexture *bufferTexture = RwTextureCreate(raster);
    if (!bufferTexture) {
        return 0;
    }

    CPlayerPed *pPed = new (std::nothrow) CPlayerPed(208, 0, 0.0f, 0.0f, 0.0f, 0.0f);
    if (!pPed) {
        return 0;
    }

    float posZ = iModel == 162 ? 50.15f : 50.05f;
    float posY = fZoom * -2.25f;
    pPed->TeleportTo(0.0f, posY, posZ);
    pPed->SetModelIndex(iModel);
    pPed->SetGravityProcessing(false);
    pPed->SetCollisionChecking(false);

    CMatrix mat;
    pPed->GetMatrix(&mat);

    if (vecRot->x != 0.0f)
    {
        CCommon::RwMatrixRotate(&mat, 0, vecRot->x);
    }

    if (vecRot->y != 0.0f)
    {
        CCommon::RwMatrixRotate(&mat, 1, vecRot->y);
    }

    if (vecRot->z != 0.0f)
    {
        CCommon::RwMatrixRotate(&mat, 2, vecRot->z);
    }

    pPed->UpdateMatrix(mat);

    m_camera->frameBuffer = raster;
    CCommon::SetRenderWareCamera(m_camera);

    RwCameraClear(m_camera, (RwRGBA*)&dwColor, 3);
    RwCameraBeginUpdate(m_camera);

    CCommon::rpWorldAddLight(m_light);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
    CCommon::DefinedState();

    pPed->Add();

    pPed->ClumpUpdateAnimations(100.0f, 1);
    pPed->Render();

    RwCameraEndUpdate(m_camera);

    CCommon::rpWorldRemoveLight(m_light);

    pPed->Remove();
    reinterpret_cast<void(*)(CPedGta*)>(*reinterpret_cast<void**>(pPed->GetPed()->vtable + 0x8))(pPed->GetPed());

    if (!CCommon::ModelInfoGetReferenceCount(iModel))
    {
        CStreaming::RemoveModel(iModel, false);
    }

    return bufferTexture;
}

/*
RwTexture* CModelSnapshot::CreateVehicleSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom, int dwColor1, int dwColor2)
{
	RwRaster *raster = RwRasterCreate(256, 256, 0, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	if (!raster) {
		return 0;
	}

	RwTexture *bufferTexture = RwTextureCreate(raster);
	if (!bufferTexture) {
		return 0;
	}

	if (iModel == 570)
	{
		iModel = 538; // eSpecialVehicle::TRAIN_PASSENGER_LOCO
	}
	else if (iModel == 569)
	{
		iModel = 537; // eSpecialVehicle::TRAIN_FREIGHT_LOCO
	}

	CVector vecPos;
	vecPos.x = 0.0f;
	vecPos.y = 0.0f;
	vecPos.z = 50.0f;

	CVehicle *pVehicle = new (std::nothrow) CVehicle(iModel, vecPos, 0.0f);
	if (!pVehicle) {
		return 0;
	}

	pVehicle->SetGravityProcessing(false);
	pVehicle->SetCollisionChecking(false);

	float fRadius = CCommon::ModelInfoGetColSphereRadius(iModel);
	float posY = (-1.0 - (fRadius + fRadius)) * fZoom;

	if (pVehicle->GetVehicleSubtype() == eVehicleSubtype::VEHICLE_SUBTYPE_BOAT)
	{
		posY = -5.5 - fRadius * 2.5;
	}

	pVehicle->TeleportTo(0.0f, posY, 50.0f);

	if (dwColor1 != 0xFFFF && dwColor2 != 0xFFFF)
	{
		pVehicle->SetColor(dwColor1, dwColor2);
	}

	CMatrix mat;
	pVehicle->GetMatrix(&mat);

	if (vecRot->x != 0.0f)
	{
		CCommon::RwMatrixRotate(&mat, 0, vecRot->x);
	}

	if (vecRot->y != 0.0f)
	{
		CCommon::RwMatrixRotate(&mat, 1, vecRot->y);
	}

	if (vecRot->z != 0.0f)
	{
		CCommon::RwMatrixRotate(&mat, 2, vecRot->z);
	}

	pVehicle->UpdateMatrix(mat);

	m_camera->frameBuffer = raster;
	CCommon::SetRenderWareCamera(m_camera);

	RwCameraClear(m_camera, (RwRGBA*)&dwColor, 3);
	RwCameraBeginUpdate(m_camera);

	CCommon::rpWorldAddLight(m_light);

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
	CCommon::DefinedState();

	pVehicle->Add();
	pVehicle->Render();

	RwCameraEndUpdate(m_camera);

	CCommon::rpWorldRemoveLight(m_light);

	pVehicle->Remove();
	SAFE_DELETE(pVehicle);

	return bufferTexture;
}

RwTexture* CModelSnapshot::CreateObjectSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom)
{
	RwRaster *raster = RwRasterCreate(256, 256, 0, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	if (!raster) {
		return 0;
	}

	RwTexture *bufferTexture = RwTextureCreate(raster);
	if (!bufferTexture) {
		return 0;
	}

	if (iModel == 1373 || iModel == 3118 || iModel == 3552 || iModel == 3553)
	{
		iModel = eSpecialObject::OBJECT_NOMODELFILE;
	}

	bool bNeedRemoveModel = false;
	if (!CStreaming::IsModelLoaded(iModel))
	{
		bNeedRemoveModel = true;
		CStreaming::RequestModel(iModel);
		CStreaming::LoadAllRequestedModels(false);

		int retries = 0;
		while (!CStreaming::IsModelLoaded(iModel))
		{
			usleep(1000);
			retries++;
			if (retries > 200)
			{
				break;
			}
		}
	}

	uintptr_t pRwObject = CCommon::ModelInfoCreateInstance(iModel);
	if (!pRwObject) {
		return 0;
	}

	float fRadius = CCommon::ModelInfoGetColSphereRadius(iModel);

	CVector vecCenter = { 0.0f, 0.0f, 0.0f };
	CCommon::ModelInfoGetColSphereVecCenter(iModel, &vecCenter);

	RwFrame* parent = *reinterpret_cast<RwFrame**>(pRwObject + 8);
	if (parent == 0)
	{
		return 0;
	}

	RwV3d v = { -vecCenter.x, (-0.1f - fRadius * 2.25f) * fZoom, 50.0f - vecCenter.z };
	CUtil::RwFrameTranslate(parent, &v, (RwOpCombineType)1);

	if (iModel == eSpecialObject::OBJECT_NOMODELFILE)
	{
		v.x = 0.0f;
		v.y = 0.0f;
		v.z = 1.0f;
		RwFrameRotateAMZ(parent, &v, 180.0f, (RwOpCombineType)1);
	}
	else
	{
		if (vecRot->x != 0.0f)
		{
			v.x = 1.0f;
			v.y = 0.0f;
			v.z = 0.0f;
			RwFrameRotateAMZ(parent, &v, vecRot->x, (RwOpCombineType)1);
		}

		if (vecRot->y != 0.0f)
		{
			v.x = 0.0f;
			v.y = 1.0f;
			v.z = 0.0f;
			RwFrameRotateAMZ(parent, &v, vecRot->y, (RwOpCombineType)1);
		}

		if (vecRot->z != 0.0f)
		{
			v.x = 0.0f;
			v.y = 0.0f;
			v.z = 1.0f;
			RwFrameRotateAMZ(parent, &v, vecRot->z, (RwOpCombineType)1);
		}
	}

	m_camera->frameBuffer = raster;
	CCommon::SetRenderWareCamera(m_camera);

	RwCameraClear(m_camera, (RwRGBA*)&dwColor, 3);
	RwCameraBeginUpdate(m_camera);

	CCommon::rpWorldAddLight(m_light);

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODENASHADEMODE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);
	CCommon::DefinedState();

	bool bShouldProcessGeometries = true;
	if (IS_VEHICLE_MOD(iModel))
	{
		bShouldProcessGeometries = false;
	}

	CBaseModelInfo* modelInfo = reinterpret_cast<CBaseModelInfo*>(CCommon::GetModelInfoByID(iModel));
	bool bIsObjectModel = false;
	if (modelInfo && CCommon::IsObjectModel(modelInfo))
	{
		bIsObjectModel = true;
	}

	std::vector<SavedGeometry> savedGeoms;
	if (bShouldProcessGeometries)
	{
		CObject::ProcessObjectGeometries(pRwObject, savedGeoms, bIsObjectModel);
	}

	CCommon::RenderClumpOrAtomic(pRwObject);

	if (bShouldProcessGeometries)
	{
		CObject::RestoreObjectGeometries(savedGeoms);
	}

	RwCameraEndUpdate(m_camera);

	CCommon::rpWorldRemoveLight(m_light);

	CCommon::DestroyAtomicOrClump(pRwObject);

	if (bNeedRemoveModel)
	{
		CStreaming::RemoveModel(iModel, false);
	}

	return bufferTexture;
}
*/