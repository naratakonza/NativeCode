#!/usr/bin/env python3

import argparse
from copy import copy
from hashlib import sha256
from pathlib import Path, PurePosixPath
import plistlib
from stat import S_IFREG
from struct import unpack_from
import tarfile
from uuid import uuid4
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


APP = "Payload/gtasa.app/"
FRAMEWORKS = APP + "Frameworks/"
ENGINE = "55eae6864b296dd9f43b2cc7577ec256e5c32a8d"
MORGAN_FLUTTER = "MorganFlutter.framework/"
LEGACY_MORGAN_LOADING = "MorganLoading.framework/"


def verified_framework(archive_path, framework):
    with tarfile.open(archive_path) as archive:
        files = {}
        for member in archive.getmembers():
            parts = PurePosixPath(member.name).parts
            if not parts or parts[0] != framework or ".." in parts or "\\" in member.name:
                raise ValueError(f"Unexpected framework member {member.name}")
            if member.isdir():
                continue
            if not member.isfile() or member.name in files:
                raise ValueError(f"Invalid framework member {member.name}")
            files[member.name] = (archive.extractfile(member).read(), member.mode)
    expected = {}
    for line in archive_path.with_name("SHA256SUMS").read_text().splitlines():
        digest, name = line.split(maxsplit=1)
        expected[name] = digest
    for name in (framework + "/" + framework.removesuffix(".framework"), framework + "/Info.plist"):
        assert sha256(files[name][0]).hexdigest() == expected[name], name
    return files


def arm64_slice(binary):
    # Flutter's XCFramework can store its single device architecture in a fat container.
    if binary[:4] == bytes.fromhex("ca fe ba be"):
        _, count, cpu, _, offset, length, _ = unpack_from(">7I", binary)
        assert count == 1 and cpu == 0x0100000C, "Expected one arm64 slice"
        assert 28 <= offset and offset + length <= len(binary), "Invalid slice bounds"
        binary = binary[offset:offset+length]
    assert unpack_from("<II", binary) == (0xFEEDFACF, 0x0100000C), "Expected arm64 Mach-O"
    return binary


def macho_commands(binary):
    assert unpack_from("<II", binary) == (0xFEEDFACF, 0x0100000C), "Expected arm64 Mach-O"
    assert unpack_from("<I", binary, 12)[0] == 6, "Expected MH_DYLIB"
    position = 32
    for _ in range(unpack_from("<I", binary, 16)[0]):
        command, size = unpack_from("<II", binary, position)
        assert size >= 8 and position + size <= len(binary)
        yield command, position, size
        position += size


def snapshot_header(binary):
    binary = arm64_slice(binary)
    segments = []
    symtab = None
    for command, pos, _ in macho_commands(binary):
        if command == 0x19:
            segments.append(unpack_from("<QQQQ", binary, pos + 24))
        elif command == 2:
            symtab = unpack_from("<IIII", binary, pos + 8)
    assert symtab, "Missing AOT symbol table"
    symbol_offset, symbol_count, string_offset, _ = symtab
    for index in range(symbol_count):
        name, kind, _, _, address = unpack_from("<IBBHQ", binary, symbol_offset + 16 * index)
        if kind & 0x0E != 0x0E or not binary.startswith(b"_kDartIsolateSnapshotData\0", string_offset + name):
            continue
        for vmaddr, _, fileoff, filesize in segments:
            if vmaddr <= address < vmaddr + filesize:
                start = address - vmaddr + fileoff
                assert binary[start:start+4] == bytes.fromhex("f5 f5 dc dc")
                # The serialized size differs. Magic, kind, snapshot version and feature flags must agree.
                end = binary.index(b"\0", start + 52)
                return binary[start:start+4] + binary[start+12:end]
    raise ValueError("Missing Dart isolate snapshot data")


def package(baseline, digest, launcher, module, output, render_title=False, timer_pause=False, cd_stream_fix=False, radar_blips=False, img_slots=False, img_read_fix=False, camera_size=False, cdraw=False, thunder_main=False, stock_player_weapon=False, clothes_rebuild=False, world_player_slots=False, cj_walk=False, physical=False, gyro=False, stats=False, widget_player_info=False, spawn_fade=False, player_ped_ctor=False, gun_movement=False, ambient_audio=False, hud=False, aim_patches=False, texture_load_fix=False, *, bass_sdk=None, widgets=False, pad_process_control=False, shadow_style=False, weapon_camera_radar=False, radar_gang_zones=False, crash_fix=False, texture_database=False, raster_alpha=False, world_player_slot_reuse=False, font_current_string=False, ped_method_hooks=False, ped_camera_streaming=False):
    assert not output.exists(), output
    assert sha256(baseline.read_bytes()).hexdigest() == digest, "Unexpected baseline IPA"
    launcher_files = verified_framework(launcher, "MorganLauncher.framework")
    module_files = verified_framework(module, MORGAN_FLUTTER.rstrip("/"))

    loading_binary = module_files[MORGAN_FLUTTER + "MorganFlutter"][0]
    loading_macho = arm64_slice(loading_binary)
    ids = [
        loading_macho[pos+unpack_from("<I", loading_macho, pos+8)[0]:pos+size].split(b"\0", 1)[0]
        for command, pos, size in macho_commands(loading_macho) if command == 0xD
    ]
    assert ids == [b"@rpath/MorganFlutter.framework/MorganFlutter"], ids
    loading_info = plistlib.loads(module_files[MORGAN_FLUTTER + "Info.plist"][0])
    assert loading_info["CFBundleIdentifier"] == "com.morgangame.ios.flutter"
    assert loading_info["CFBundleExecutable"] == "MorganFlutter"
    assert loading_info["MorganFlutterEngine"] == ENGINE
    assert not any(command == 0x1D for command, _, _ in macho_commands(loading_macho))
    # codesign removes the Mach-O signature but can leave the resource seal in the framework.
    module_files.pop(MORGAN_FLUTTER + "_CodeSignature/CodeResources", None)
    assert all("_CodeSignature" not in PurePosixPath(name).parts for name in module_files)
    assert MORGAN_FLUTTER + "flutter_assets/AssetManifest.bin" in module_files
    assert MORGAN_FLUTTER + "flutter_assets/assets/backgroud01.png" in module_files

    launcher_binary = "MorganLauncher.framework/MorganLauncher"
    launcher_data = launcher_files[launcher_binary][0]
    list(macho_commands(launcher_data))
    assert b"MorganFlutter.framework\0" in launcher_data, "Launcher does not reference MorganFlutter.framework"
    replacement_name = FRAMEWORKS + launcher_binary
    replacement_files = {FRAMEWORKS + name: data for name, (data, _) in module_files.items()}
    replacement_files[replacement_name] = launcher_data
    temporary = output.with_name("." + uuid4().hex + ".ipa")
    try:
        with ZipFile(baseline) as original:
            names = original.namelist()
            assert len(names) == len(set(names))
            assert original.testzip() is None
            engine_info = plistlib.loads(original.read(FRAMEWORKS + "Flutter.framework/Info.plist"))
            assert engine_info["FlutterEngine"] == ENGINE
            rockstar_aot = original.read(FRAMEWORKS + "App.framework/App")
            baseline_snapshot = snapshot_header(rockstar_aot)
            module_snapshot = snapshot_header(loading_binary)
            assert baseline_snapshot == module_snapshot, (baseline_snapshot, module_snapshot)
            launcher_plist = "MorganLauncher.framework/Info.plist"
            assert plistlib.loads(original.read(FRAMEWORKS + launcher_plist)) == plistlib.loads(launcher_files[launcher_plist][0])
            assert replacement_name in names
            existing_module = {name for name in names if name.startswith(FRAMEWORKS + MORGAN_FLUTTER)}
            legacy_module = {name for name in names if name.startswith(FRAMEWORKS + LEGACY_MORGAN_LOADING)}
            retained_names = set(names) - legacy_module
            expected_module = set(replacement_files) - {replacement_name}
            assert not existing_module or existing_module == expected_module
            if bass_sdk is not None:
                from bass_ios import bass_framework_files

                bass_files = bass_framework_files(bass_sdk)
                framework_files = module_files | bass_files
                replacement_files.update({FRAMEWORKS + name: data for name, (data, _) in bass_files.items()})
            else:
                framework_files = module_files
            additions = {FRAMEWORKS + name for name in framework_files} - retained_names

            if render_title:
                from patch_render_title import PAUSE_SYMBOLS, SYMBOL, patch_render_title

                assert SYMBOL + b"\0" in launcher_data, "Launcher lacks the end-of-frame title callback"
                if timer_pause:
                    assert all(symbol + b"\0" in launcher_data for symbol in PAUSE_SYMBOLS), "Launcher lacks timer pause callbacks"
                executable = APP + "gtasa"
                replacement_files[executable] = patch_render_title(original.read(executable), timer_pause=timer_pause)

            if cd_stream_fix:
                from patch_cd_stream import patch_cd_stream

                executable = APP + "gtasa"
                replacement_files[executable] = patch_cd_stream(original.read(executable))

            if radar_blips:
                from patch_radar_blips import SYMBOLS, patch_radar_blips

                assert all(symbol + b"\0" in launcher_data for symbol in SYMBOLS), "Launcher lacks the radar bridges"
                executable = APP + "gtasa"
                replacement_files[executable] = patch_radar_blips(replacement_files[executable])

            if img_slots or img_read_fix:
                from patch_img_slots import OLD_SYMBOL, SYMBOLS, patch_img_slots

                assert all(symbol + b"\0" in launcher_data for symbol in SYMBOLS), "Launcher lacks the IMG bridges"
                assert OLD_SYMBOL + b"\0" not in launcher_data, "Launcher still contains the extra IMG loader"
                executable = APP + "gtasa"
                if img_read_fix:
                    from patch_img_slots import patch_img_read_failures

                    replacement_files[executable] = patch_img_read_failures(original.read(executable))
                else:
                    replacement_files[executable] = patch_img_slots(original.read(executable))

            if camera_size:
                from patch_camera_size import SYMBOL, patch_camera_size

                assert SYMBOL + b"\0" in launcher_data, "Launcher lacks the CameraSize bridge"
                executable = APP + "gtasa"
                replacement_files[executable] = patch_camera_size(original.read(executable))

            if cdraw:
                from patch_cdraw import SYMBOL, patch_cdraw

                assert SYMBOL + b"\0" in launcher_data, "Launcher lacks the CDraw SetFOV bridge"
                executable = APP + "gtasa"
                replacement_files[executable] = patch_cdraw(original.read(executable))

            if thunder_main:
                from patch_thunder_main import patch_thunder_main

                executable = APP + "gtasa"
                replacement_files[executable] = patch_thunder_main(original.read(executable), player_weapon=not stock_player_weapon)

            if clothes_rebuild:
                from patch_clothes_rebuild import SYMBOL, patch_clothes_rebuild

                assert SYMBOL + b"\0" in launcher_data, "Launcher lacks the clothes rebuild bridge"
                executable = APP + "gtasa"
                replacement_files[executable] = patch_clothes_rebuild(original.read(executable))

            if world_player_slots:
                from patch_world_player_slots import FIND_PLAYER_SLOT_SYMBOL, patch_world_player_slots

                if FIND_PLAYER_SLOT_SYMBOL + b"\0" not in launcher_data:
                    raise ValueError("Launcher lacks CWorld::FindPlayerSlotWithPedPointer")
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_world_player_slots(source if source is not None else original.read(executable))

            if cj_walk:
                from patch_cj_walk import patch_cj_walk

                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_cj_walk(source if source is not None else original.read(executable))

            if physical:
                from patch_physical import SYMBOL, patch_physical

                if SYMBOL + b"\0" not in launcher_data:
                    raise ValueError("Launcher lacks CPhysical::ProcessControl")
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_physical(source if source is not None else original.read(executable))

            if gyro:
                from patch_gyro import HOOKS, patch_gyro

                if any(symbol + b"\0" not in launcher_data for _, _, symbol in HOOKS):
                    raise ValueError("Launcher lacks the CPad gyro hooks")
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_gyro(source if source is not None else original.read(executable))
                app_plist = APP + "Info.plist"
                app_plist_data = original.read(app_plist)
                app_info = plistlib.loads(app_plist_data)
                if "NSMotionUsageDescription" not in app_info:
                    app_info["NSMotionUsageDescription"] = "Use the gyroscope to control the camera and aim when motion controls are enabled."
                    plist_format = plistlib.FMT_BINARY if app_plist_data.startswith(b"bplist00") else plistlib.FMT_XML
                    replacement_files[app_plist] = plistlib.dumps(app_info, fmt=plist_format, sort_keys=False)

            if stats:
                from patch_stats import SYMBOL, patch_stats

                if SYMBOL + b"\0" not in launcher_data:
                    raise ValueError("Launcher lacks the CStats hook")
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_stats(source if source is not None else original.read(executable))

            if widget_player_info:
                from patch_widget_player_info import SYMBOL, patch_widget_player_info

                if SYMBOL + b"\0" not in launcher_data:
                    raise ValueError("Launcher lacks the CWidgetPlayerInfo hook")
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_widget_player_info(source if source is not None else original.read(executable))

            if spawn_fade:
                from patch_spawn_fade import patch_spawn_fade

                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_spawn_fade(source if source is not None else original.read(executable))

            if player_ped_ctor:
                from patch_player_ped_ctor import patch_player_ped_ctor

                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_player_ped_ctor(source if source is not None else original.read(executable))

            if gun_movement:
                from patch_gun_movement import patch_gun_movement

                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_gun_movement(source if source is not None else original.read(executable))

            if ambient_audio:
                from patch_ambient_audio import SYMBOL, patch_ambient_audio

                assert SYMBOL + b"\0" in launcher_data, "Launcher lacks the ambient audio hook"
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_ambient_audio(source if source is not None else original.read(executable))

            if hud:
                from patch_hud import HOOKS, patch_hud

                assert all(symbol + b"\0" in launcher_data for symbol, _, _ in HOOKS), "Launcher lacks the HUD hooks"
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_hud(source if source is not None else original.read(executable))

            if aim_patches:
                from patch_aim import patch_aim

                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_aim(source if source is not None else original.read(executable))

            if texture_load_fix:
                from patch_texture_load import SYMBOL, patch_texture_load

                assert SYMBOL + b"\0" in launcher_data, "Launcher lacks the LoadFullTexture hook"
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_texture_load(source if source is not None else original.read(executable))

            if widgets:
                from patch_widgets import HOOKS, patch_widgets

                assert all(symbol + b"\0" in launcher_data for _, _, symbol, _ in HOOKS), "Launcher lacks the CWidgets hooks"
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_widgets(source if source is not None else original.read(executable))

            if pad_process_control:
                from patch_pad_process_control import SHADOW_SYMBOLS, SYMBOLS, patch_pad_process_control

                symbols = SYMBOLS + (SHADOW_SYMBOLS if shadow_style else ())
                assert all(symbol + b"\0" in launcher_data for symbol in symbols), "Launcher lacks the requested CPad or shadow hooks"
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_pad_process_control(source if source is not None else original.read(executable), shadow_style=shadow_style)

            if weapon_camera_radar:
                from patch_weapon_camera_radar import patch_weapon_camera_radar

                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_weapon_camera_radar(source if source is not None else original.read(executable))

            if radar_gang_zones or crash_fix or texture_database:
                from patch_radar_gang_zones import HOOKS, CRASH_FIX_HOOKS, TEXTURE_DATABASE_HOOKS, patch_radar_gang_zones
                hooks = (HOOKS if radar_gang_zones else ()) + (CRASH_FIX_HOOKS if crash_fix else ()) + (TEXTURE_DATABASE_HOOKS if texture_database else ())
                assert all(symbol + b"\0" in launcher_data for _, _, symbol in hooks), "Launcher lacks the selected radar or crash fix bridges"
                executable = APP + "gtasa"
                source = replacement_files.get(executable)
                replacement_files[executable] = patch_radar_gang_zones(source if source is not None else original.read(executable), radar=radar_gang_zones, crash_fix=crash_fix, texture_database=texture_database)

            if raster_alpha:
                executable = APP + "gtasa"
                source = replacement_files.get(executable, original.read(executable))
                # RQRenderTarget::Create(width, height, colour=1, depth=1).
                site = 0x28AA60
                expected = bytes.fromhex("020080522300805267780094A01200F9")
                patched = bytes.fromhex("22008052") + expected[4:]
                if source[site:site + len(expected)] not in (expected, patched):
                    raise ValueError("Unexpected iOS RasterCreate render-target instructions")
                replacement_files[executable] = source[:site] + patched[:4] + source[site + 4:]

            if world_player_slot_reuse:
                from patch_world_player_slots import patch_world_player_slot_reuse

                executable = APP + "gtasa"
                source = replacement_files.get(executable, original.read(executable))
                replacement_files[executable] = patch_world_player_slot_reuse(source)

            if font_current_string:
                from patch_font_current_string import patch_font_current_string

                executable = APP + "gtasa"
                source = replacement_files.get(executable, original.read(executable))
                replacement_files[executable] = patch_font_current_string(source)

            if ped_method_hooks:
                from patch_ped_methods import SYMBOLS, patch_ped_methods

                if any(symbol + b"\0" not in launcher_data for symbol in SYMBOLS):
                    raise ValueError("Launcher lacks the civilian control or player collision method hook")
                executable = APP + "gtasa"
                source = replacement_files.get(executable, original.read(executable))
                replacement_files[executable] = patch_ped_methods(source)

            if ped_camera_streaming:
                from patch_ped_camera_streaming import patch_ped_camera_streaming

                executable = APP + "gtasa"
                source = replacement_files.get(executable, original.read(executable))
                replacement_files[executable] = patch_ped_camera_streaming(source)

            with ZipFile(temporary, "x") as ipa:
                ipa.comment = original.comment
                for member in original.infolist():
                    if member.filename in legacy_module:
                        continue
                    data = replacement_files[member.filename] if member.filename in replacement_files else original.read(member)
                    ipa.writestr(copy(member), data)
                for name, (data, mode) in sorted(framework_files.items()):
                    if FRAMEWORKS + name not in additions:
                        continue
                    member = ZipInfo(FRAMEWORKS + name)
                    member.create_system = 3
                    member.external_attr = (S_IFREG | mode) << 16
                    ipa.writestr(member, data, ZIP_DEFLATED)

            with ZipFile(temporary) as ipa:
                assert ipa.testzip() is None
                assert len(ipa.namelist()) == len(retained_names) + len(additions)
                assert set(ipa.namelist()) == retained_names | additions
                assert all(ipa.read(name) == original.read(name) for name in retained_names if name not in replacement_files)
                assert all(ipa.getinfo(name).external_attr == original.getinfo(name).external_attr for name in retained_names)
                assert all(ipa.read(name) == data for name, data in replacement_files.items())
        temporary.rename(output)
    finally:
        temporary.unlink(missing_ok=True)

    print(output)
    print("IPA SHA-256", sha256(output.read_bytes()).hexdigest())
    print("Dart AOT snapshot version and feature flags match the embedded Rockstar app.")
    print("ZIP CRC and member comparison passed. Sign the test IPA with ESign before installation.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Assemble a test IPA with the Morgan Flutter loading screen.")
    parser.add_argument("baseline", type=Path)
    parser.add_argument("baseline_sha256")
    parser.add_argument("launcher_tar", type=Path)
    parser.add_argument("flutter_tar", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--render-title", action="store_true", help="Install the title render bridge in the GTA test executable.")
    parser.add_argument("--timer-pause", action="store_true", help="Bind the user pause callbacks in the GTA test executable.")
    parser.add_argument("--cd-stream-fix", action="store_true", help="Apply the verified iOS CD streaming repair to the test executable.")
    parser.add_argument("--radar-blips", action="store_true", help="Pass world coordinates to the iOS radar hooks.")
    parser.add_argument("--img-slots", action="store_true", help="Use the native GTA directory pass for all 32 IMG slots.")
    parser.add_argument("--img-read-fix", action="store_true", help="Repair unrecoverable CD requests on the verified IMG32 baseline.")
    parser.add_argument("--camera-size", action="store_true", help="Route gameplay CameraSize through Morgan on the verified IMG Read Fix baseline.")
    parser.add_argument("--cdraw", action="store_true", help="Route gameplay SetFOV through Morgan on the verified CameraSize baseline.")
    parser.add_argument("--thunder-main", action="store_true", help="Apply the three Thunder main patches on the verified CDraw baseline.")
    parser.add_argument("--stock-player-weapon", action="store_true", help="Keep the stock weapon branch when applying the Thunder main patches.")
    parser.add_argument("--clothes-rebuild", action="store_true", help="Route player rebuild calls through Morgan on the verified Sensitivity baseline.")
    parser.add_argument("--world-player-slots", action="store_true", help="Extend GTA's engine player records on the verified iOS baseline.")
    parser.add_argument("--cj-walk", action="store_true", help="Enable Thunder movement-group and FollowPedSA head-look patches on the verified World Player Slots and Clothes Rebuild executable.")
    parser.add_argument("--physical", action="store_true", help="Redirect CPhysical::ProcessControl on the verified executable with CJ walk and HeadMove.")
    parser.add_argument("--gyro", action="store_true", help="Enable the CPad aim hooks on the verified Physical executable.")
    parser.add_argument("--stats", action="store_true", help="Enable Thunder's health modifiers on the verified Gyro executable.")
    parser.add_argument("--widget-player-info", action="store_true", help="Enable Thunder's armor bar on the verified CStats executable.")
    parser.add_argument("--spawn-fade", action="store_true", help="Apply the CCamera::Fade RET patch on the verified Widget Player Info executable.")
    parser.add_argument("--player-ped-ctor", action="store_true", help="Flip the CPlayerPed player group branch on the verified SpawnFade executable.")
    parser.add_argument("--gun-movement", action="store_true", help="Apply Thunder's movement constants while aiming on the verified SpawnFade executable.")
    parser.add_argument("--ambient-audio", action="store_true", help="Route the weather audio callback through Morgan on the verified SpawnFade executable.")
    parser.add_argument("--hud", action="store_true", help="Enable Thunder's stock crosshair positioning and player info alpha on the verified AmbientAudio executable.")
    parser.add_argument("--aim-patches", action="store_true", help="Apply Thunder's targeting patches to the verified AmbientAudio executable.")
    parser.add_argument("--texture-load-fix", action="store_true", help="Enable bounded DAT reads and RLE decoding on the verified HUD and AimPatches executable.")
    parser.add_argument("--widgets", action="store_true", help="Enable CWidgets on the verified ShowGameText executable.")
    parser.add_argument("--pad-process-control", action="store_true", help="Enable the CPad ProcessControl and UpdatePosition hooks on the verified Widgets executable.")
    parser.add_argument("--bass-sdk", type=Path, help="Embed the verified Un4seen BASS iPhoneOS framework from the SDK ZIP.")
    parser.add_argument("--shadow-style", action="store_true", help="Enable the Classic/Advance shadow controls with --pad-process-control.")
    parser.add_argument("--weapon-camera-radar", action="store_true", help="Apply the sniper shake, resurrection camera and GPS sprite patches on the verified Pickup executable.")
    parser.add_argument("--radar-gang-zones", action="store_true", help="Install verified iOS radar/GangZone bridges and Thunder map/colour patches on Keybar25 baseline.")
    parser.add_argument("--crash-fix", action="store_true", help="Guard RenderWare material and texture destruction on the verified Keybar25 executable.")
    parser.add_argument("--texture-database", action="store_true", help="Install the iOS TextureDatabaseRuntime GetTexture fallback hook.")
    parser.add_argument("--raster-alpha", action="store_true", help="Use colour type 1 for iOS camera-texture render targets, matching Thunder.")
    parser.add_argument("--world-player-slot-reuse", action="store_true", help="Release the previous remote slot record before CREATE_PLAYER replaces its Ped.")
    parser.add_argument("--font-current-string", action="store_true", help="Expand the verified iOS CFont::ProcessCurrentString buffer to match Thunder.")
    parser.add_argument("--ped-method-hooks", action="store_true", help="Bind the verified civilian ProcessControl and player ProcessCollision vtable slots.")
    parser.add_argument("--ped-camera-streaming", action="store_true", help="NOP the verified weapon-target camera and post-death entity deletion calls.")
    arguments = parser.parse_args()
    if arguments.shadow_style and not arguments.pad_process_control:
        parser.error("--shadow-style requires --pad-process-control")
    if arguments.timer_pause and not arguments.render_title:
        parser.error("--timer-pause requires --render-title")
    if arguments.cd_stream_fix and arguments.render_title:
        parser.error("--cd-stream-fix expects a baseline that already contains the render title bridge")
    if arguments.radar_blips and not arguments.cd_stream_fix:
        parser.error("--radar-blips requires --cd-stream-fix")
    if arguments.img_slots and (arguments.render_title or arguments.cd_stream_fix):
        parser.error("--img-slots expects the RadarMod IPA with the earlier patches already present")
    if arguments.img_read_fix and (arguments.render_title or arguments.cd_stream_fix or arguments.img_slots):
        parser.error("--img-read-fix expects the IMG32 IPA with the earlier patches already present")
    if arguments.camera_size and (arguments.render_title or arguments.cd_stream_fix or arguments.img_slots or arguments.img_read_fix):
        parser.error("--camera-size expects the IMG Read Fix IPA with the earlier patches already present")
    if arguments.cdraw and (arguments.render_title or arguments.cd_stream_fix or arguments.img_slots or arguments.img_read_fix or arguments.camera_size):
        parser.error("--cdraw expects the CameraSize IPA with the earlier patches already present")
    if arguments.thunder_main and (arguments.render_title or arguments.cd_stream_fix or arguments.img_slots or arguments.img_read_fix or arguments.camera_size or arguments.cdraw):
        parser.error("--thunder-main expects the CDraw IPA with the earlier patches already present")
    if arguments.stock_player_weapon and not arguments.thunder_main:
        parser.error("--stock-player-weapon requires --thunder-main")
    if arguments.clothes_rebuild and (arguments.render_title or arguments.cd_stream_fix or arguments.img_slots or arguments.img_read_fix or arguments.camera_size or arguments.cdraw or arguments.thunder_main):
        parser.error("--clothes-rebuild expects the Sensitivity IPA with the earlier patches already present")
    package(arguments.baseline, arguments.baseline_sha256, arguments.launcher_tar, arguments.flutter_tar, arguments.output,
            arguments.render_title, arguments.timer_pause, arguments.cd_stream_fix, arguments.radar_blips, arguments.img_slots, arguments.img_read_fix, arguments.camera_size, arguments.cdraw, arguments.thunder_main, arguments.stock_player_weapon, arguments.clothes_rebuild, arguments.world_player_slots, arguments.cj_walk, arguments.physical, arguments.gyro, arguments.stats, arguments.widget_player_info, arguments.spawn_fade, arguments.player_ped_ctor, arguments.gun_movement, arguments.ambient_audio, arguments.hud, arguments.aim_patches, arguments.texture_load_fix, bass_sdk=arguments.bass_sdk, widgets=arguments.widgets, pad_process_control=arguments.pad_process_control, shadow_style=arguments.shadow_style, weapon_camera_radar=arguments.weapon_camera_radar, radar_gang_zones=arguments.radar_gang_zones, crash_fix=arguments.crash_fix, texture_database=arguments.texture_database, raster_alpha=arguments.raster_alpha, world_player_slot_reuse=arguments.world_player_slot_reuse, font_current_string=arguments.font_current_string, ped_method_hooks=arguments.ped_method_hooks, ped_camera_streaming=arguments.ped_camera_streaming)
