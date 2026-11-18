#include "../../../Main.h"

#include "RenderWare.h"

#include <mach-o/dyld.h>

RsGlobalType* RsGlobal;
RwInt32 (*RwEngineGetCurrentVideoMode)();
RwVideoMode* (*RwEngineGetVideoModeInfo)(RwVideoMode* modeInfo, RwInt32 modeIndex);

RwCamera *(*RwCameraBeginUpdate)(RwCamera *camera);
RwCamera *(*RwCameraEndUpdate)(RwCamera *camera);
RwCamera *(*RwCameraShowRaster)(RwCamera *camera, void *pDev, RwUInt32 flags);
RwCamera *(*RwCameraCreate)(void);
RwCamera *(*RwCameraSetFarClipPlane)(RwCamera *camera, float far);
RwCamera *(*RwCameraSetNearClipPlane)(RwCamera *camera, float far);
RwCamera *(*RwCameraSetViewWindow)(RwCamera *camera, RwV2d const& view);
RwCamera *(*RwCameraSetProjection)(RwCamera *, RwCameraProjection);
RwCamera *(*RwCameraClear)(RwCamera *, RwRGBA *, int);

RwFrame* (*RwFrameCreate)(void);
RwFrame* (*RwFrameTranslateAMZ)(RwFrame* frame, const RwV3d* v, RwOpCombineType combine);
RwFrame* (*RwFrameRotateAMZ)(RwFrame* frame, const RwV3d* axis, RwReal angle, RwOpCombineType combine);
RwFrame* (*RwFrameForAllObjects)(RwFrame* frame, RwObjectCallBack callBack, void* data);

RpMaterial* (*RpMaterialCreate)(void);

RwObject *(*RwObjectHasFrameSetFrame)(void *camera, RwFrame *frame);
RwBool (*RwCameraDestroy)(RwCamera * camera);
RwBool (*RwFrameDestroy)(RwFrame * frame);

RpWorld *(*RpWorldAddCamera)(RpWorld *, RwCamera *);
RpWorld *(*RpWorldRemoveCamera)(RpWorld *, RwCamera *);
RpWorld* (*RpWorldAddLight)(RpWorld* world, RpLight* light);
RpWorld* (*RpWorldRemoveLight)(RpWorld* world, RpLight* light);

RpLight* (*RpLightCreate)(RwInt32 type);
RpLight* (*RpLightSetColor)(RpLight* light, const RwRGBAReal* color);

RwRaster *(*RwRasterCreate)(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
RwBool (*RwRasterDestroy)(RwRaster * raster);
RwRaster *(*RwRasterGetOffset)(RwRaster *raster, RwInt16 *xOffset, RwInt16 *yOffset);
RwInt32 (*RwRasterGetNumLevels)(RwRaster * raster);
RwRaster *(*RwRasterSubRaster)(RwRaster * subRaster, RwRaster * raster, RwRect * rect);
RwRaster *(*RwRasterRenderFast)(RwRaster * raster, RwInt32 x, RwInt32 y);
RwRaster *(*RwRasterRender)(RwRaster * raster, RwInt32 x, RwInt32 y);
RwRaster *(*RwRasterRenderScaled)(RwRaster * raster, RwRect * rect);
RwRaster *(*RwRasterPushContext)(RwRaster * raster);
RwRaster *(*RwRasterPopContext)(void);
RwRaster *(*RwRasterGetCurrentContext)(void);
RwBool (*RwRasterClear)(RwInt32 pixelValue);
RwBool (*RwRasterClearRect)(RwRect * rpRect, RwInt32 pixelValue);
RwRaster *(*RwRasterShowRaster)(RwRaster * raster, void *dev, RwUInt32 flags);
RwUInt8 *(*RwRasterLock)(RwRaster * raster, RwUInt8 level, RwInt32 lockMode);
RwRaster *(*RwRasterUnlock)(RwRaster * raster);
RwUInt8 *(*RwRasterLockPalette)(RwRaster * raster, RwInt32 lockMode);
RwRaster *(*RwRasterUnlockPalette)(RwRaster * raster);
RwImage *(*RwImageCreate)(RwInt32 width, RwInt32 height, RwInt32 depth);
RwBool (*RwImageDestroy)(RwImage * image);
RwImage *(*RwImageAllocatePixels)(RwImage * image);
RwImage *(*RwImageFreePixels)(RwImage * image);
RwImage *(*RwImageCopy)(RwImage * destImage, const RwImage * sourceImage);
RwImage *(*RwImageResize)(RwImage * image, RwInt32 width, RwInt32 height);
RwImage *(*RwImageApplyMask)(RwImage * image, const RwImage * mask);
RwImage *(*RwImageMakeMask)(RwImage * image);
RwImage *(*RwImageReadMaskedImage)(const RwChar * imageName, const RwChar * maskname);
RwImage *(*RwImageRead)(const RwChar * imageName);
RwImage *(*RwImageWrite)(RwImage * image, const RwChar * imageName);
RwImage *(*RwImageSetFromRaster)(RwImage *image, RwRaster *raster);
RwRaster *(*RwRasterSetFromImage)(RwRaster *raster, RwImage *image);
RwRaster *(*RwRasterRead)(const RwChar *filename);
RwRaster *(*RwRasterReadMaskedRaster)(const RwChar *filename, const RwChar *maskname);
RwImage *(*RwImageFindRasterFormat)(RwImage *ipImage, RwInt32 nRasterType, RwInt32 *npWidth, RwInt32 *npHeight, RwInt32 *npDepth, RwInt32 *npFormat);
RwTexture* (*RwTextureRead)(const char*, const char*);
RwTexture *(*RwTextureCreate)(RwRaster* raster);
RwBool (*RwTextureDestroy)(RwTexture* texture);
RwTexDictionary* (*RwTexDictionaryGetCurrent)(void);
RwTexture* (*RwTexDictionaryFindNamedTexture)(RwTexDictionary* dict, const char* name);
RwTexture* (*TextureDatabaseRuntime_GetRWTexture)(void* db, int entry_index);
RwStream* (*RwStreamOpen)(RwStreamType streamType, RwStreamAccessType accessType, const void* data);
RwBool (*RwStreamFindChunk)(RwStream* stream, RwUInt32 type, RwUInt32* length, RwUInt32* version);
RpClump* (*RpClumpStreamRead)(RwStream* stream);
RwUInt32 (*RwStreamRead)(RwStream* stream, void* buffer, RwUInt32 length);
RpClump* (*RpClumpClone)(RpClump* clump);
RwBool (*RwStreamClose)(RwStream* stream, void* data);
RpAtomic* (*GetFirstAtomic)(RpClump* clump);
void        (*RpGeometryLock)(RpGeometry* geo, int a);
RpGeometry* (*RpGeometryUnlock)(RpGeometry* geo);
RpAtomic* (*RpAtomicClone)(RpAtomic* atomic);
RwBool (*RpClumpDestroy)(RpClump* clump);
RpAtomic* (*RpAtomicSetFrame)(RpAtomic* atomic, RwFrame* frame);


/* rwlpcore.h */
RwReal (*RwIm2DGetNearScreenZ)(void);
RwReal (*RwIm2DGetFarScreenZ)(void);
RwBool (*RwRenderStateGet)(RwRenderState state, void *value);
RwBool (*RwRenderStateSet)(RwRenderState state, void *value);
RwBool (*RwIm2DRenderLine)(RwIm2DVertex *vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2);
RwBool (*RwIm2DRenderTriangle)(RwIm2DVertex *vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2, RwInt32 vert3 );
RwBool (*RwIm2DRenderPrimitive)(RwPrimitiveType primType, RwIm2DVertex *vertices, RwInt32 numVertices);
RwBool (*RwIm2DRenderIndexedPrimitive)(RwPrimitiveType primType, RwIm2DVertex *vertices, RwInt32 numVertices, RwImVertexIndex *indices, RwInt32 numIndices);

RxObjSpace3DVertex *(*RwIm3DTransform)(RxObjSpace3DVertex *pVerts, RwUInt32 numVerts, RwMatrix *ltm, RwUInt32 flags);
RwBool (*RwIm3DRenderPrimitive)(RwPrimitiveType primType);
RwBool (*RwIm3DRenderIndexedPrimitive)(RwPrimitiveType primType, RwUInt16 *indices, RwInt32 numIndices);
RwBool (*RwIm3DEnd)(void);

/* rtpng.h */
RwImage *(*RtPNGImageWrite)(RwImage* image, const RwChar* imageName);
RwImage *(*RtPNGImageRead)(const RwChar* imageName);

RpGeometry *(*RpGeometryForAllMaterials)(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData);
RpMaterial* (*RpMaterialSetTexture)(RpMaterial* material, RwTexture* texture);
RpClump* (*RpClumpForAllAtomics)(RpClump* clump, RpAtomicCallBack fpCallBack, void* pData);
RwBool (*RwResourcesFreeResEntry)(RwResEntry* entry);

template<typename Func>
static bool SetFuncAddr(Func* function, std::uint64_t address, const char* expected)
{
    if (!CMemory::HasMappedInstruction(address, expected) || !CMemory::HasMappedInstruction(address + 4, expected + 4)) {
        *function = nullptr;
        return false;
    }

    *function = reinterpret_cast<Func>(address + _dyld_get_image_vmaddr_slide(0));
    return true;
}

void InitialiseRenderWare()
{
    CNecessary::Log(OBFUSCATE("Initializing RenderWare.."));

    RsGlobal = CMemory::HasMappedInstruction(SA_ADDR(0x31558C), "\x08\x30\x00\xF0") && CMemory::HasMappedInstruction(SA_ADDR(0x315590), "\x08\xA1\x29\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x315594), "\x00\x05\x00\xFD") ? reinterpret_cast<RsGlobalType*>(SA_ADDR(0x918A68) + _dyld_get_image_vmaddr_slide(0)) : nullptr;

    // เฉพาะรายการที่ตรงกับ GTA iOS นี้เท่านั้นที่จะถูกเผยแพร่ ส่วนตัวชี้ (pointer) อื่นๆ จะยังคงเป็นค่าว่าง (null)
    unsigned bound = 0;
    bound += RsGlobal != nullptr;
    bound += SetFuncAddr(&RwEngineGetCurrentVideoMode, SA_ADDR(0x2AC26C), "\xFF\xC3\x00\xD1\xFD\x7B\x02\xA9");
    bound += SetFuncAddr(&RwEngineGetVideoModeInfo, SA_ADDR(0x2AC1E4), "\xFF\xC3\x00\xD1\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwCameraBeginUpdate, SA_ADDR(0x2AAE90), "\x01\x18\x40\xF9\x20\x00\x1F\xD6");
    bound += SetFuncAddr(&RwCameraEndUpdate, SA_ADDR(0x2AAE88), "\x01\x1C\x40\xF9\x20\x00\x1F\xD6");
    bound += SetFuncAddr(&RwCameraSetNearClipPlane, SA_ADDR(0x2AAECC), "\xE9\x23\xBD\x6D\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwCameraSetFarClipPlane, SA_ADDR(0x2AAF78), "\xE9\x23\xBD\x6D\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwCameraClear, SA_ADDR(0x2AB158), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwCameraShowRaster, SA_ADDR(0x2AB190), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwCameraSetProjection, SA_ADDR(0x2AB1BC), "\xFF\x03\x01\xD1\xE9\x23\x01\x6D");
    bound += SetFuncAddr(&RwCameraSetViewWindow, SA_ADDR(0x2AB2C0), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwCameraDestroy, SA_ADDR(0x2AB340), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwCameraCreate, SA_ADDR(0x2AB39C), "\xE9\x23\xBD\x6D\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwFrameCreate, SA_ADDR(0x2ACEA8), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwFrameDestroy, SA_ADDR(0x2AD150), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwFrameTranslateAMZ, SA_ADDR(0x2AD42C), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwFrameRotateAMZ, SA_ADDR(0x2AD544), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwFrameForAllObjects, SA_ADDR(0x2AD728), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RwImageCreate, SA_ADDR(0x2ADC5C), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwImageDestroy, SA_ADDR(0x2ADCD0), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwImageFreePixels, SA_ADDR(0x2ADD4C), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwImageAllocatePixels, SA_ADDR(0x2ADD94), "\xFF\x03\x01\xD1\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RwImageMakeMask, SA_ADDR(0x2ADE8C), "\x09\x0C\x40\xB9\x3F\x11\x00\x71");
    bound += SetFuncAddr(&RwImageApplyMask, SA_ADDR(0x2AE014), "\xFF\xC3\x01\xD1\xFC\x6F\x01\xA9");
    bound += SetFuncAddr(&RwImageCopy, SA_ADDR(0x2AE3C4), "\xFF\x43\x01\xD1\xF8\x5F\x01\xA9");
    bound += SetFuncAddr(&RwImageRead, SA_ADDR(0x2AE5BC), "\xFF\x03\x01\xD1\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RwImageWrite, SA_ADDR(0x2AEA0C), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RwImageReadMaskedImage, SA_ADDR(0x2AEAF0), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwImageSetFromRaster, SA_ADDR(0x2AEF08), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRasterSetFromImage, SA_ADDR(0x2AEF44), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwImageFindRasterFormat, SA_ADDR(0x2AEF80), "\xFF\x43\x02\xD1\xF8\x5F\x05\xA9");
    bound += SetFuncAddr(&RwRasterUnlock, SA_ADDR(0x2AF018), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRasterRenderFast, SA_ADDR(0x2AF058), "\xFF\xC3\x00\xD1\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwRasterDestroy, SA_ADDR(0x2AF0E0), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRasterPushContext, SA_ADDR(0x2AF150), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRasterLockPalette, SA_ADDR(0x2AF1F0), "\xFF\x83\x00\xD1\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRasterPopContext, SA_ADDR(0x2AF234), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRasterGetNumLevels, SA_ADDR(0x2AF29C), "\xFF\x83\x00\xD1\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRasterShowRaster, SA_ADDR(0x2AF2F8), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwRasterCreate, SA_ADDR(0x2AF350), "\xFA\x67\xBB\xA9\xF8\x5F\x01\xA9");
    bound += SetFuncAddr(&RwRasterLock, SA_ADDR(0x2AF430), "\xFF\x83\x00\xD1\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwTextureDestroy, SA_ADDR(0x2B03EC), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9") ||
        SetFuncAddr(&RwTextureDestroy, SA_ADDR(0x2B03EC), "\x57\x9D\x07\x14\xFD\x7B\x01\xA9") ||
        SetFuncAddr(&RwTextureDestroy, SA_ADDR(0x2B03EC), "\x2F\x9D\x07\x14\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwTextureCreate, SA_ADDR(0x2B04E8), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwTexDictionaryFindNamedTexture, SA_ADDR(0x2B0714), "\x08\x0C\x41\xF8\x1F\x01\x00\xEB");
    bound += SetFuncAddr(&RwTexDictionaryGetCurrent, SA_ADDR(0x2B07B4), "\x88\x2A\x00\xB0\x08\xA1\x3C\x91");
    bound += SetFuncAddr(&RwTextureRead, SA_ADDR(0x2B07D4), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwStreamFindChunk, SA_ADDR(0x2B6FA4), "\xFF\x43\x01\xD1\xF6\x57\x02\xA9");
    bound += SetFuncAddr(&RwIm2DGetNearScreenZ, SA_ADDR(0x2B7794), "\x48\x2A\x00\xD0\x08\xA1\x3C\x91");
    bound += SetFuncAddr(&RwIm2DGetFarScreenZ, SA_ADDR(0x2B77A8), "\x48\x2A\x00\xD0\x08\xA1\x3C\x91");
    bound += SetFuncAddr(&RwRenderStateSet, SA_ADDR(0x2B77BC), "\xFF\x83\x00\xD1\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwRenderStateGet, SA_ADDR(0x2B7818), "\x48\x2A\x00\xD0\x08\xA1\x3C\x91");
    bound += SetFuncAddr(&RwIm2DRenderIndexedPrimitive, SA_ADDR(0x2B7868), "\x48\x2A\x00\xD0\x08\xA1\x3C\x91");
    bound += SetFuncAddr(&RwResourcesFreeResEntry, SA_ADDR(0x2B9CF8), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RwStreamRead, SA_ADDR(0x2BA288), "\xFF\x03\x01\xD1\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RwStreamClose, SA_ADDR(0x2BA71C), "\xFF\xC3\x00\xD1\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RwStreamOpen, SA_ADDR(0x2BA850), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RtPNGImageRead, SA_ADDR(0x2C1614), "\xFF\x83\x02\xD1\xF8\x5F\x06\xA9");
    bound += SetFuncAddr(&RpClumpForAllAtomics, SA_ADDR(0x2C2DFC), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RpAtomicClone, SA_ADDR(0x2C3288), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RpClumpClone, SA_ADDR(0x2C3444), "\xFF\x83\x01\xD1\xFA\x67\x01\xA9");
    bound += SetFuncAddr(&RpClumpDestroy, SA_ADDR(0x2C35CC), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RpClumpStreamRead, SA_ADDR(0x2C3A80), "\xFF\x03\x03\xD1\xFC\x6F\x06\xA9");
    bound += SetFuncAddr(&RpGeometryUnlock, SA_ADDR(0x2C4244), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RpGeometryForAllMaterials, SA_ADDR(0x2C4988), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RpLightCreate, SA_ADDR(0x2C53D8), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RpMaterialCreate, SA_ADDR(0x2C577C), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RpMaterialSetTexture, SA_ADDR(0x2C587C), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RpWorldAddCamera, SA_ADDR(0x2CA5BC), "\xF6\x57\xBD\xA9\xF4\x4F\x01\xA9");
    bound += SetFuncAddr(&RpWorldRemoveCamera, SA_ADDR(0x2CA604), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RpWorldAddLight, SA_ADDR(0x2CA874), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RpWorldRemoveLight, SA_ADDR(0x2CA8DC), "\xF8\x5F\xBC\xA9\xF6\x57\x01\xA9");
    bound += SetFuncAddr(&RwRasterUnlockPalette, SA_ADDR(0x2AF0A0), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");
    bound += SetFuncAddr(&RpAtomicSetFrame, SA_ADDR(0x2C3108), "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9");

    CNecessary::Log(OBFUSCATE("RenderWare iOS bindings: %u/73"), bound);
}