#include "../../../Main.h"
#include "CPatches.h"
#include "CUvAnim.h"
#include "../Models/CModelInfo.h"
#include "../Entity/Ped/CPlayerPed.h"
#include "Thread/CFastLoad.h"

#include <mach-o/dyld.h>

void CPatches::InstallLoadFilePatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallLoadFilePatches"));

    // โหลดฐานเท็กซ์เจอร์ผ่าน InitialiseRenderWare_hook แล้วข้ามคำสั่งโหลดชุดเดิม
    CMemory::WriteProcessMemory(SA_ADDR(0x152748), (uintptr_t)"\x74\x12\x0D\x94", 4); // mobile
    CMemory::makeNOP(SA_ADDR(0x15275C), 1); // txd
    CMemory::makeNOP(SA_ADDR(0x152770), 1); // gta3
    CMemory::makeNOP(SA_ADDR(0x152784), 1); // gta_int
    CMemory::makeNOP(SA_ADDR(0x152798), 1); // cutscene

    CMemory::WriteProcessMemory(SA_ADDR(0x086FE0), (uintptr_t)"\x00\x00\x80\x52", 4); // ข้ามการเปิด CINFO.BIN เพื่ออ่าน
    CMemory::WriteProcessMemory(SA_ADDR(0x22ED18), (uintptr_t)"\x00\x00\x80\x52", 4); // ข้ามการเปิด MINFO.BIN เพื่ออ่าน

    CMemory::WriteProcessMemory(SA_ADDR(0x087290), (uintptr_t)"\x39\x00\x00\x14", 4); // ข้ามการเขียน CINFO.BIN และคืน buffer ตามเดิม
    CMemory::WriteProcessMemory(SA_ADDR(0x22EDA4), (uintptr_t)"\x0B\x00\x00\x14", 4); // ข้ามการเขียน MINFO.BIN และคืน buffer ตามเดิม

    CMemory::makeRET(SA_ADDR(0x0446B0)); // ปิด CCutsceneMgr::LoadCutsceneData
    CMemory::makeRET(SA_ADDR(0x0445B8)); // ปิด CCutsceneMgr::Initialise
    CMemory::WriteProcessMemory(SA_ADDR(0x046ABC), (uintptr_t)"\xDA\x00\x00\x14", 4); // ข้ามตัวโหลดคัตซีนสำรองเมื่อปิดการสร้าง directory

    char byteHudCellphoneData[14]{};
    std::memcpy(byteHudCellphoneData, OBFUSCATE("GymAccept"), sizeof("GymAccept"));
    CMemory::WriteProcessMemory(SA_ADDR(0x480D96), reinterpret_cast<uintptr_t>(byteHudCellphoneData), sizeof(byteHudCellphoneData));
}

void CPatches::InstallLevelZPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallLevelZPatches"));

    CMemory::WriteProcessMemory(SA_ADDR(0x1511D4), (uintptr_t)"\xC0\x00\x80\x52", 4); // CdStreamInit(6)

    CMemory::WriteProcessMemory(SA_ADDR(0x2A218C), (uintptr_t)"\x00\x08\xA0\x52", 4); // จัดสรร RenderQueue 4 MiB
    CMemory::WriteProcessMemory(SA_ADDR(0x2A2198), (uintptr_t)"\x08\x00\x50\x91", 4); // ตั้งปลายบัฟเฟอร์ RenderQueue ที่ base + 4 MiB

    CMemory::makeRET(SA_ADDR(0x14CACC)); // CFileLoader::LoadPickup (แก้ไขแคช)
    CMemory::makeRET(SA_ADDR(0x21CF28)); // Interior_c::AddPickups (แก้ไขแคช)
    CMemory::makeNOP(SA_ADDR(0x21D254), 1); // Interior_c::Exit ข้ามการลบ pickup ในพื้นที่และคงการคืนทรัพยากร

    // LoadFullTexture อ่าน DAT และคลาย RLE ผ่าน bridge ของ iOS
    CMemory::WriteProcessMemory(SA_ADDR(0x2BE7BC), (uintptr_t)"\x9A\x63\x07\x14", 4);

    // RasterCreate (แก้ไขบัค Alpha)
    CMemory::WriteProcessMemory(SA_ADDR(0x28AA60), (uintptr_t)"\x22\x00\x80\x52", 4);

    CMemory::makeNOP(SA_ADDR(0x43BF58), 1); // ไม่เปิดเมนูแผนที่อัตโนมัติเมื่อกลับเข้าเกม

    // ให้ Aim Indicator ใช้เส้นทาง GetInputType == 0 ของ iOS
    CMemory::WriteProcessMemory(SA_ADDR(0x3BF784), (uintptr_t)"\x1F\x00\x00\x71", 4); // CTaskSimplePlayerOnFoot::ProcessPlayerWeapon
    CMemory::WriteProcessMemory(SA_ADDR(0x2733D8), (uintptr_t)"\x1F\x00\x00\x71", 4); // CPlayerPed::Compute3rdPersonMouseTarget

    // CPlayerPed::CPlayerPed เปลี่ยน TBNZ เป็น TBZ ก่อนขั้นตั้งค่า player group
    CMemory::WriteProcessMemory(SA_ADDR(0x26CBF4), (uintptr_t)"\x54\x05\x00\x36", 4);

    // CTaskSimpleUseGun::SetMoveAnim ปรับค่าจาก 0.1 เป็น 0.015f ปรับความเร็วโยกซ้ายขวา ตอนซูม Jiggle Movement
    CMemory::WriteProcessMemory(SA_ADDR(0x34944C), (uintptr_t)"\xE8\x51\x98\x52", 4);
    CMemory::WriteProcessMemory(SA_ADDR(0x349450), (uintptr_t)"\xA8\x8E\xA7\x72", 4);

    // CTaskSimpleUseGun::ControlGunMove และ CTaskSimpleGunControl::ProcessPed ใช้ตัวคูณ 2.0f แทน 0.07f
    CMemory::WriteProcessMemory(SA_ADDR(0x349940), (uintptr_t)"\x08\x00\xA8\x52", 4);
    CMemory::makeNOP(SA_ADDR(0x349944), 1);
    CMemory::WriteProcessMemory(SA_ADDR(0x34AB2C), (uintptr_t)"\x08\x00\xA8\x52", 4);
    CMemory::makeNOP(SA_ADDR(0x34AB30), 1);

    // แก้ไข Crash CFont::ProcessCurrentString (แก้ไขให้รองรับ TextDraw String ยาวๆ)
    CMemory::WriteProcessMemory(SA_ADDR(0x2F265C), (uintptr_t)"\xFF\x03\x32\xD1", 4); // ปรับเป็น SUB SP, SP, #0xC80 (3200)
    CMemory::WriteProcessMemory(SA_ADDR(0x2F273C), (uintptr_t)"\x09\x71\x30\x91", 4); // ปรับเป็น ADD X9, X8, #0xC1C (3100)
    CMemory::WriteProcessMemory(SA_ADDR(0x2F2E40), (uintptr_t)"\xFF\x03\x32\x91", 4); // ปรับเป็น ADD SP, SP, #0xC80 (3200)
}

void CPatches::InstallSettingGamePatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallSettingGamePatches"));

    CMemory::WriteProcessMemory(SA_ADDR(0x1DDE3C), (uintptr_t)"\x2D\x00\x00\x14", 4); // ข้ามการเพิ่ม menu_mainstats
    CMemory::WriteProcessMemory(SA_ADDR(0x1DDF00), (uintptr_t)"\x2E\x00\x00\x14", 4); // ข้ามการเพิ่ม menu_mainbriefs

    CMemory::WriteProcessMemory(SA_ADDR(0x1E7588), (uintptr_t)"\x23\x00\x80\x52", 4); // แสดงเปอร์เซ็นต์บนแถบตั้งค่า

    CMemory::WriteProcessMemory(SA_ADDR(0x47F2DC), (uintptr_t)(const char*)OBFUSCATE("menu_mainquit"), sizeof("menu_mainquit"));
    CMemory::WriteProcessMemory(SA_ADDR(0x47F2EA), (uintptr_t)(const char*)OBFUSCATE("FEP_QUI"), sizeof("FEP_QUI"));
}

void CPatches::InstallSavePatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallSavePatches"));

    CMemory::makeNOP(SA_ADDR(0x43BFE8), 1); // ข้ามการเซฟจาก event pause ของ iOS

    if (!CMemory::HasMappedInstruction(SA_ADDR(0x31B130), "\x69\x0E\x00\xB0") || !CMemory::HasMappedInstruction(SA_ADDR(0x31B134), "\x29\x21\x3B\x91"))
    {
        CNecessary::Log(OBFUSCATE("CPatches default save name unavailable"));
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    char* DefaultPCSaveFileName = reinterpret_cast<char*>(SA_ADDR(0x4E8EC8) + slide);
    if (std::memcmp(DefaultPCSaveFileName, "GTASAsf", sizeof("GTASAsf")) == 0)
    {
        std::memcpy(DefaultPCSaveFileName, OBFUSCATE("MorGanZ"), sizeof("MorGanZ"));
    }
}

void CPatches::InstallTemporaryFrameratePatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallFrameratePatches"));

    // FrontEndMenuManager.m_bDisplayFPS (FrontEndMenuManager + 0xC1)
    const bool verified = CMemory::HasMappedInstruction(SA_ADDR(0x15A2B4), "\xA8\x31\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x15A2B8), "\x08\xA1\x07\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x15A2BC), "\x08\x05\x43\x39") && CMemory::HasMappedInstruction(SA_ADDR(0x15A2C4), "\x67\xFB\xFF\x97");

    if (verified)
    {
        const auto slide = _dyld_get_image_vmaddr_slide(0);
        *reinterpret_cast<std::uint8_t*>(SA_ADDR(0x78F2A9) + slide) = 1;
    }
}

void CPatches::InstallMainPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallMainPatches"));

    CMemory::makeNOP(SA_ADDR(0x43AC30), 1); // ปิดการเรียกใช้งาน CAudioEngine::StartLoadingTune จาก DoGameState

    CMemory::makeNOP(SA_ADDR(0x1E0324), 1); // ปิดการเรียก Menu_SwitchOffToGame จาก LoadScreen::LoadScreenSelection::Load

    CMemory::WriteProcessMemory(SA_ADDR(0x15F3FC), (uintptr_t)"\x00\x00\x80\x52", 4); // คืน false ให้ CPad::GetDisplayVitalStats
    CMemory::makeRET(SA_ADDR(0x15F400)); // ปิดการเรียก widget Vital Stats ผ่านอินพุตเดิม
    CMemory::WriteProcessMemory(SA_ADDR(0x15F468), (uintptr_t)"\x00\x00\x80\x52", 4); // คืน false ให้ CPad::GetDisplayVitalStats(CPed*)
    CMemory::makeRET(SA_ADDR(0x15F46C));

    CNecessary::Log(OBFUSCATE("MORGAN MAIN PATCHES 1"));

    CMemory::makeRET(SA_ADDR(0x178948)); // CStats::DisplayScriptStatUpdateMessage
    CMemory::makeRET(SA_ADDR(0x1F3D6C)); // CRadar::DrawLegend
    CMemory::makeRET(SA_ADDR(0x1F3A6C)); // CRadar::AddBlipToLegendList
    CMemory::makeRET(SA_ADDR(0x216680)); // CHud::DrawVehicleName
    CMemory::makeRET(SA_ADDR(0x192BF4)); // CPlaceName::Process
    CMemory::makeRET(SA_ADDR(0x20F330)); // CWidgetRegionSteeringSelection::Draw
    CMemory::makeRET(SA_ADDR(0x2027F8)); // CWidgetHelpText::Process
    CMemory::makeRET(SA_ADDR(0x215094)); // CHud::SetHelpMessage
    CMemory::makeRET(SA_ADDR(0x215134)); // CHud::SetHelpMessageStatUpdate
    CMemory::makeRET(SA_ADDR(0x21761C)); // CHud::DrawBustedWastedMessage

    // ข้าม CVehicleRecording::Load โดยไปทางที่ RemoveModel และปิด RwStream
    CMemory::WriteProcessMemory(SA_ADDR(0x308828), (uintptr_t)"\x49\x00\x00\x14", 4);

    CMemory::WriteProcessMemory(SA_ADDR(0x1909BC), (uintptr_t)"\x00\x00\x80\xD2", 4); // คืน nullptr ให้ CPlayerInfo::FindObjectToSteal
    CMemory::makeRET(SA_ADDR(0x1909C0)); // CPlayerInfo::FindObjectToSteal (ปิดระบบยก Object)

    // ปิดการเรียกใช้ Radio เสียงเพลงตอนอยู่บนรถ
    CMemory::makeRET(SA_ADDR(0x05F1D4)); // CAERadioTrackManager::StartRadio

    CNecessary::Log(OBFUSCATE("MORGAN MAIN PATCHES 2"));

    CMemory::makeRET(SA_ADDR(0x0AF190)); // CCheat::AddToCheatString
    CMemory::makeRET(SA_ADDR(0x0AF124)); // CCheat::DoCheats
    CMemory::makeRET(SA_ADDR(0x0ACC80)); // CCheat::WeaponCheat1
    CMemory::makeRET(SA_ADDR(0x0ACF40)); // CCheat::WeaponCheat2
    CMemory::makeRET(SA_ADDR(0x0AD1C4)); // CCheat::WeaponCheat3
    CMemory::makeRET(SA_ADDR(0x0AD3D4)); // CCheat::WeaponCheat4

    CNecessary::Log(OBFUSCATE("MORGAN MAIN PATCHES 3"));

    CMemory::makeRET(SA_ADDR(0x0D591C)); // CRoadBlocks::GenerateRoadBlocks

    // ปิด C_PcSave::SaveSlot และคืนรหัสเซฟล้มเหลวของเกม
    CMemory::WriteProcessMemory(SA_ADDR(0x31C0C0), (uintptr_t)"\x40\x00\x80\x52", 4);
    CMemory::makeRET(SA_ADDR(0x31C0C4));

    // CPopulation::AddPed ยังใช้สร้าง ped จากสคริปต์
    CMemory::WriteProcessMemory(SA_ADDR(0x27CF90), (uintptr_t)"\x00\x00\x80\xD2", 4); // ข้าม AddPed จาก AddPedAtAttractor
    CMemory::WriteProcessMemory(SA_ADDR(0x276DFC), (uintptr_t)"\x00\x00\x80\x52", 4); // ปิดเฉพาะการสร้าง NPC ทั่วไปใน CPopulation::Update
    CMemory::makeRET(SA_ADDR(0x0B6BD4)); // CEntryExit::GenerateAmbientPeds

    // ปิดไว้ก่อน เอาไว้ทดสอบระบบ ( หากจะปล่อย Release ต้องเปิด )
    // CMemory::makeRET(SA_ADDR(0x42674C)); // CTheCarGenerators::Process
    // CMemory::makeRET(SA_ADDR(0x0970C0)); // CCarCtrl::GenerateRandomCars
    // CMemory::makeRET(SA_ADDR(0x4042C0)); // CPlane::DoPlaneGenerationAndRemoval

    CMemory::WriteProcessMemory(SA_ADDR(0x2466A4), (uintptr_t)"\x00\x00\x80\xD2", 4); // คืน nullptr ให้ CPed::GiveObjectToPedToHold
    CMemory::makeRET(SA_ADDR(0x2466A8)); // CPed::GiveObjectToPedToHold (ยกเลิกการเกิดใหม่พร้อมกับซิการ์และขวด)
    CMemory::makeRET(SA_ADDR(0x1BC438)); // CEventHandler::ComputeGunAimedAtResponse

    // ข้ามการสร้างผู้โดยสารเสริมก่อนถึง CCarEnterExit::SetPedInCarDirect
    CMemory::WriteProcessMemory(SA_ADDR(0x09B2CC), (uintptr_t)"\x00\x00\x80\xD2", 4);
    CMemory::WriteProcessMemory(SA_ADDR(0x09B320), (uintptr_t)"\x00\x00\x80\xD2", 4);

    CMemory::makeRET(SA_ADDR(0x04FC44)); // CAEGlobalWeaponAudioEntity::ServiceAmbientGunFire ( เสียงยิงปืนของ NPC )
    CMemory::makeRET(SA_ADDR(0x3C0410)); // CTaskSimplePlayerOnFoot::PlayIdleAnimations

    // GetInputType แก้ไข เมาส์หลอน โดยใช้ค่าปกติเมื่อไม่มีอุปกรณ์ภายนอก
    CMemory::WriteProcessMemory(SA_ADDR(0x1CBD48), (uintptr_t)"\x00\x00\x80\x52", 4);
    CMemory::makeRET(SA_ADDR(0x1CBD4C));

    // ปิดการเรียกใช้งาน CMessages::AddBigMessage จาก CPlayerInfo::KillPlayer
    CMemory::makeNOP(SA_ADDR(0x191938), 1);

    // ปิดชื่อ Zone ในแผนที่
    CMemory::makeNOP(SA_ADDR(0x1EE7A0), 1);

    // ปิด HeadMove ใน CCam::Process_FollowPed_SA เพราะใน samp ใช้ของ sync แล้ว
    CMemory::makeNOP(SA_ADDR(0x11F134), 1);

    // ข้ามเฉพาะ CWeaponEffects::Render ใน RenderEffects
    CMemory::makeNOP(SA_ADDR(0x159B20), 1);

    // ปิดการเรียกใช้งาน CamShakeNoPos จาก CWeapon::FireSniper (ปิดหน้าจอสั่นขณะยิงสไน)
    CMemory::makeNOP(SA_ADDR(0x4334DC), 1);

    // CMirrors::RenderReflections ใช้ตำแหน่งกล้องสะท้อนคงที่ทุกโหมดแบบ Arita
    CMemory::makeNOP(SA_ADDR(0x301B14), 1);

    // บังคับให้ใช้ท่าทางจาก IDE สำหรับตัวละคร
    CPlayerPed::ProcessAnimGroups(false);

    // ปิดการเรียกใช้งาน CCamera::RestoreWithJumpCut จาก CGameLogic::RestorePlayerStuffDuringResurrection // ปิดใช้งานการตัดภาพแบบกระโดดของกล้องหลังจากเกิดใหม่
    CMemory::makeNOP(SA_ADDR(0x0B7AB4), 1);

    // แก้ไข UvAnim
    // ตัวแพ็กเปิดแฟล็กเฉพาะ RpMaterialUVAnimAddAnimTime (0x29F904) และคืนค่าเดิมหลังอัปเดต

    // CClouds::RenderSkyPolys (แก้ไขภาพ ฉีก ตอนปรับ Fov หรือ อื่นๆ)
    if (!CMemory::HasMappedBytes(SA_ADDR(0x467718), "\x00\x00\x20\xC1\x00\x00\x20\x41", 8, "__const"))
    {
        CNecessary::Log(OBFUSCATE("CPatches RenderSkyPolys constants unavailable"));
    }

    // CMemory::WriteProcessMemory(SA_ADDR(0x3BFB20), (uintptr_t)"\x07\x02\x00\x14", 4); // CTaskSimplePlayerOnFoot::ProcessPlayerWeapon (ระบบ ยืนแล้วสามารถใช้มีดอีโมได้เลย) // เดียวมาเปิดใช้งานภายหลัง
    CMemory::WriteProcessMemory(SA_ADDR(0x1593D0), (uintptr_t)"\x1C\x00\x00\x14", 4); // DoRWStuffStartOfFrame_Horizon (แก้ไขภาพ ฉีก ตอนปรับ Fov หรือ อื่นๆ) ทดสอบ
}

void CPatches::InstallGameProcessPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallGameProcessPatches"));

    CMemory::makeNOP(SA_ADDR(0x151EB0), 1); // CGangWars::Update
    CMemory::makeNOP(SA_ADDR(0x151BBC), 1); // CCover::Update
    CMemory::makeNOP(SA_ADDR(0x151C54), 1); // CDarkel::Update
    CMemory::makeNOP(SA_ADDR(0x151C68), 1); // CSetPieces::Update
    CMemory::makeNOP(SA_ADDR(0x151E5C), 1); // CCarCtrl::PruneVehiclesOfInterest
    CMemory::makeNOP(SA_ADDR(0x151E9C), 1); // CInterestingEvents::ScanForNearbyEntities
    CMemory::makeNOP(SA_ADDR(0x151EB4), 1); // CConversations::Update
    CMemory::makeNOP(SA_ADDR(0x151EB8), 1); // CPedToPlayerConversations::Update
    CMemory::makeNOP(SA_ADDR(0x151EFC), 1); // CCarCtrl::RemoveCarsIfThePoolGetsFull
    CMemory::makeNOP(SA_ADDR(0x151C50), 1); // CHeli::UpdateHelis
    CMemory::makeNOP(SA_ADDR(0x151F98), 1); // CHeli::UpdateHelis (เส้นทาง pause)
    CMemory::makeNOP(SA_ADDR(0x151B88), 1); // CCutsceneMgr::Update
    CMemory::makeNOP(SA_ADDR(0x151EC0), 1); // CCoronas::DoSunAndMoon
    CMemory::makeNOP(SA_ADDR(0x151EE0), 1); // CPlantMgr::Update
}

void CPatches::InstallGraphicsOptimizationPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallGraphicsOptimizationPatches"));

    CMemory::makeRET(SA_ADDR(0x173A6C)); // CRopes::Render
    CMemory::makeRET(SA_ADDR(0x2F9A88)); // CSkidmarks::Render
    CMemory::makeRET(SA_ADDR(0x2F99CC)); // CSkidmarks::Update
    // CMemory::makeRET(SA_ADDR(0x2F3EE0)); // CGlass::Render
    CMemory::makeRET(SA_ADDR(0x2FBDA4)); // CMotionBlurStreaks::Update
    CMemory::makeRET(SA_ADDR(0x13A628)); // CCamera::RenderMotionBlur
    // CMemory::makeRET(SA_ADDR(0x144308)); // CEntity::PreRenderForGlassWindow
}

void CPatches::InstallPadPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallPadPatches"));

    // สำหรับ ONFOOT
    CMemory::WriteProcessMemory(SA_ADDR(0x15ECD8), (uintptr_t)"\x00\x00\x80\x52", 4); // CPad::GetDuck คืน 0
    CMemory::makeRET(SA_ADDR(0x15ECDC));
    CMemory::WriteProcessMemory(SA_ADDR(0x15F330), (uintptr_t)"\x00\x00\x80\x52", 4); // CPad::SprintJustDown คืน 0
    CMemory::makeRET(SA_ADDR(0x15F334));
    CMemory::WriteProcessMemory(SA_ADDR(0x15E000), (uintptr_t)"\x00\x00\x80\x52", 4); // CPad::GetMeleeAttack คืน 0
    CMemory::makeRET(SA_ADDR(0x15E004));

    // สำหรับ INCAR (เดียวมาเปิดภายหลัง)
    // CMemory::makeRET(SA_ADDR(0x49E7B0)); // CPad::GetAccelerateJustDown
    // CMemory::makeRET(SA_ADDR(0x49DC30)); // CPad::GetExitVehicle
    // CMemory::makeRET(SA_ADDR(0x49F278)); // CPad::GetBunnyHopDown
}

void CPatches::InstallATZPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallATZPatches"));

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    const bool zonesVerified = CMemory::HasMappedInstruction(SA_ADDR(0x1934D8), "\xC8\x2F\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x1934DC), "\x1F\x79\x0E\xB9") && CMemory::HasMappedInstruction(SA_ADDR(0x1934E0), "\xC8\x2F\x00\xB0") && CMemory::HasMappedInstruction(SA_ADDR(0x1934E4), "\x08\x49\x38\x91") && CMemory::HasMappedInstruction(SA_ADDR(0x1934F8), "\x1F\x61\x00\xB9");
    if (zonesVerified)
    {
        std::memset(reinterpret_cast<void*>(SA_ADDR(0x78CE12) + slide), 1, 100); // CTheZones::ZonesVisited
        *reinterpret_cast<std::uint32_t*>(SA_ADDR(0x78CE78) + slide) = 100; // CTheZones::ZonesRevealed
    }

    CMemory::makeNOP(SA_ADDR(0x24BBE4), 1); // CPed::Say คืน -1 ผ่านทาง no-event เดิม

    const bool fovVerified = CMemory::HasMappedInstruction(SA_ADDR(0x11CC08), "\x28\x1E\x00\xD0") && CMemory::HasMappedInstruction(SA_ADDR(0x11CC0C), "\x01\x39\x40\xBD");
    if (fovVerified)
    {
        auto* carFovStartSpeed = reinterpret_cast<float*>(SA_ADDR(0x4E2038) + slide);
        if (*carFovStartSpeed == 0.4f)
        {
            *carFovStartSpeed = 200.0f;
        }
    }
}

void CPatches::InstallRadarPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallRadarPatches"));

    CMemory::makeNOP(SA_ADDR(0x1EEE18), 1); // ซ่อนเฉพาะข้อความ Legend บนแผนที่

    // ปิดการเรียกใช้งาน CSprite2d::Draw ไม่ให้แสดงผล Marker GPS
    CMemory::makeNOP(SA_ADDR(0x1F21E0), 1); 
    CMemory::makeNOP(SA_ADDR(0x1F2DA0), 1);

    // CRadar::Draw3dMarkers (PlayerColor)
    CMemory::WriteProcessMemory(SA_ADDR(0x1F42C0), (uintptr_t)"\xE1\x03\x14\x2A", 4); // mov w1, w20
    CMemory::WriteProcessMemory(SA_ADDR(0x1F42C4), (uintptr_t)"\x1F\x00\x00\x14", 4); // B 0x1F4340

    // CRadar::DrawEntityBlip (PlayerColor)
    CMemory::WriteProcessMemory(SA_ADDR(0x1F2810), (uintptr_t)"\xE1\x03\x16\x2A", 4); // mov w1, w22
    CMemory::WriteProcessMemory(SA_ADDR(0x1F2814), (uintptr_t)"\x22\x00\x00\x14", 4); // B 0x1F289C

    // CRadar::DrawCoordBlip (PlayerColor)
    CMemory::WriteProcessMemory(SA_ADDR(0x1F1E30), (uintptr_t)"\xE1\x03\x16\x2A", 4); // mov w1, w22
    CMemory::WriteProcessMemory(SA_ADDR(0x1F1E34), (uintptr_t)"\x0F\x00\x00\x14", 4); // B 0x1F1E70

    // แก้ไข Marker ให้สามารถ mark ได้ในด้านขวาของหน้าจอ (ตัวหารแกน X จาก 448.0f เป็น 240.0f)
    CMemory::WriteProcessMemory(SA_ADDR(0x1ED22C), (uintptr_t)"\x09\x6E\xA8\x52", 4); // mov w9, #0x43700000 (240.0f)
}

void CPatches::InstallDisableAutoAimPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallDisableAutoAimPatches"));

    CMemory::makeRET(SA_ADDR(0x270DC0)); // CPlayerPed::FindWeaponLockOnTarget
    CMemory::makeRET(SA_ADDR(0x271FA4)); // CPlayerPed::FindNextWeaponLockOnTarget
    CMemory::makeRET(SA_ADDR(0x246C38)); // CPed::SetWeaponLockOnTarget
}

void CPatches::InstallCameraPatches()
{
    CNecessary::Log(OBFUSCATE("CPatches InstallCameraPatches"));

    CMemory::WriteProcessMemory(SA_ADDR(0x1308C4), (uintptr_t)"\x1F\x05\x00\x71", 4); // CCamera::CamControl เปรียบเทียบโหมดกับ 1
}

void CPatches::SetPatches(ePatchType type)
{
    CNecessary::Log(OBFUSCATE("InstallSetPatches type : %d"), static_cast<int>(type));

    switch (type) {
        case ePatchType::PATCH_TYPE_ZERO: {
            CPatches::InstallLoadFilePatches();
            CPatches::InstallLevelZPatches();
            CPatches::InstallSettingGamePatches();
            CFastLoad::Initialise();
            CModelInfo::InstallModelPatches();

            break;
        }

        case ePatchType::PATCH_TYPE_MENU: {
            CPatches::InstallSavePatches();
            CPatches::InstallTemporaryFrameratePatches();
            CPatches::InstallMainPatches();
            CPatches::InstallGameProcessPatches();
            CPatches::InstallGraphicsOptimizationPatches();
            CPatches::InstallPadPatches();
            CUvAnim::Initialise();

            break;
        }

        case ePatchType::PATCH_TYPE_GAME: {
            CPatches::InstallATZPatches();
            CPatches::InstallRadarPatches();
            CPatches::InstallDisableAutoAimPatches();
            CPatches::InstallCameraPatches();

            break;
        }
    }
}