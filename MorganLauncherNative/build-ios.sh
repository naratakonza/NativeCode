#!/usr/bin/env bash
set -euo pipefail
trap 'printf "build-ios.sh failed at line %s: %s\\n" "$LINENO" "$BASH_COMMAND" >&2' ERR

if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo 'Usage: bash build-ios.sh OUTPUT_DIRECTORY /path/to/NativeSource [Debug|Release]' >&2
    exit 64
fi
if [[ "$(uname -s)" != Darwin ]] || ! command -v xcrun >/dev/null; then
    echo 'BUILD BLOCKED: macOS with Xcode and the iPhoneOS SDK is required.' >&2
    exit 2
fi

configuration="${3:-Release}"
link_flags=()
case "$configuration" in
    Debug)
        fileio_flags=(-O0 -g)
        launcher_flags=(-O0 -g)
        ;;
    Release)
        fileio_flags=(-O2 -g -DNDEBUG -ffunction-sections -fdata-sections)
        launcher_flags=(-O2 -g -ffunction-sections -fdata-sections)
        # Keep SetGameTime and GetActiveInterior linked for this runtime port.
        link_flags=(-Wl,-dead_strip -Wl,-u,__ZN6CClock11SetGameTimeEii)
        ;;
    *)
        echo 'Configuration must be Debug or Release.' >&2
        exit 64
        ;;
esac

project="$(cd "$(dirname "$0")" && pwd)"
if [[ "$configuration" == Release ]]; then
    checkout="$(cd "$project/.." && pwd)"
    fileio_flags+=("-fdebug-prefix-map=$checkout=.")
    launcher_flags+=("-fdebug-prefix-map=$checkout=.")
fi
native="$(cd "$2" && pwd)"
launcher="$native/Launcher"
fileio="$native/source/utils/FileIO"
storage="$native/source/utils/Storage"
memory="$native/source/utils/Memory"
hooks="$native/source/game_morgan/Base"
multiplayer="$native/source/game_morgan/Multiplayer"
imgui="$native/source/vendor/imgui"
scripts="$native/source/game_morgan/Scripts"
radar="$native/source/game_morgan/RadarMod"
[[ -f "$launcher/MGLauncherViewController.h" && -f "$launcher/MGLauncherViewController.mm" ]]
sdk="$(xcrun --sdk iphoneos --show-sdk-path)"
mkdir -p "$1"
out="$(cd "$1" && pwd)"
framework="$out/MorganLauncher.framework"
dylib="$framework/MorganLauncher"
install_name="@rpath/MorganLauncher.framework/MorganLauncher"
icon="$out/icon"
dsym="$out/MorganLauncher.framework.dSYM"
if [[ -e "$framework" || -e "$icon" || -e "$dsym" ]]; then
    echo 'Use a fresh output directory for each build.' >&2
    exit 1
fi

mkdir "$framework"
cp "$project/FrameworkInfo.plist" "$framework/Info.plist"
plutil -lint "$framework/Info.plist"
[[ "$(plutil -extract CFBundleExecutable raw -o - "$framework/Info.plist")" == MorganLauncher ]]
[[ "$(plutil -extract CFBundleIdentifier raw -o - "$framework/Info.plist")" == com.morgangame.ios.launcher ]]

xcodebuild -version
xcrun --sdk iphoneos clang++ --version
mkdir "$out/fileio-objects"
compile_failed=0
for source in \
    "$native/source/game_morgan/CCameraAim.cpp" "$native/source/game_morgan/CAspectRatio.cpp" "$native/source/game_morgan/CCameraController.cpp" "$native/source/game_morgan/CDraw.cpp" "$native/source/game_morgan/CSensitivity.cpp" "$native/source/game_morgan/CGyro.cpp" \
    "$native/source/game_morgan/CdStreamInfo.cpp" \
    "$native/source/game_morgan/CCommon.cpp" "$native/source/game_morgan/Entity/CEntity.cpp" "$native/source/game_morgan/Entity/CPhysical.cpp" "$native/source/game_morgan/CStats.cpp" "$native/source/game_morgan/CWeapon.cpp" \
    "$native/source/game_morgan/FrontEnd/CWidgetPlayerInfo.cpp" "$native/source/game_morgan/CSprite2d.cpp" "$native/source/game_morgan/CRGBA.cpp" "$native/source/game_morgan/CHud.cpp" \
    "$native/source/game_morgan/CPedIntelligence.cpp" "$native/source/game_morgan/Tasks/CTaskManager.cpp" "$native/source/game_morgan/Tasks/CTask.cpp" "$native/source/game_morgan/Tasks/TaskTypes/CTaskSimpleJetPack.cpp" "$native/source/game_morgan/Tasks/TaskTypes/CTaskSimplePlayerOnFoot.cpp" \
    "$native/source/game_morgan/Animation/CAnimManager.cpp" "$native/source/game_morgan/Animation/CAnimation.cpp" "$native/source/game_morgan/Core/CKeyGen.cpp" "$native/source/game_morgan/Entity/Actor/CActor.cpp" "$multiplayer/Pools/Process/CActorPool.cpp" "$multiplayer/Guis/CTextDraw.cpp" "$multiplayer/Pools/Process/CTextDrawPool.cpp" \
    "$native/source/game_morgan/CRadar.cpp" "$native/source/game_morgan/CGangZone.cpp" "$multiplayer/Pools/Process/CGangZonePool.cpp" \
    "$native/source/game_morgan/CPickup.cpp" "$multiplayer/Pools/Process/CPickupPool.cpp" \
    "$native/source/game_morgan/Entity/Ped/CPed.cpp" "$native/source/game_morgan/Entity/Ped/CPlayerPed.cpp" "$native/source/game_morgan/Entity/Ped/CPlayerPedSlot.cpp" "$native/source/game_morgan/Entity/Ped/CPedPointerRecorder.cpp" \
    "$native/source/game_morgan/CStreaming.cpp" "$native/source/game_morgan/CClock.cpp" "$native/source/game_morgan/CTimer.cpp" "$native/source/game_morgan/COptimizeDistance.cpp" "$native/source/game_morgan/CGame.cpp" "$native/source/game_morgan/Models/CModelInfo.cpp" "$native/source/game_morgan/CPools.cpp" "$native/source/game_morgan/CSetting.cpp" "$multiplayer/CServer.cpp" "$multiplayer/CNetGame.cpp" "$native/source/game_morgan/CTheZones.cpp" "$native/source/game_morgan/CWeather.cpp" "$native/source/game_morgan/CCamera.cpp" "$hooks/CPad.cpp" \
    "$native/source/game_morgan/CTouchInterface.cpp" \
    "$hooks/Graphics/CRealTimeShadow.cpp" "$hooks/Graphics/CRealTimeShadowManager.cpp" "$hooks/Graphics/CShadowCamera.cpp" \
    "$scripts/CScripting.cpp" "$scripts/CRunningScript.cpp" "$native/source/utils/Exceptions/CExceptions.cpp" \
    "$native/source/utils/Quat/CQuaternion.cpp" \
    "$radar/CTiltedRadar.cpp" "$radar/CTiltedCameraController.cpp" "$radar/CTiltedRadarGeometry.cpp" "$radar/CTiltedRadarMath.cpp" \
    "$native/Main.cpp" "$native/source/game_morgan/RenderWare/RenderWare.cpp" "$multiplayer/CMultiPlayer.cpp" "$native/source/utils/Necessary/CNecessary.cpp" \
    "$native/source/game_morgan/CMainMenuScreen.cpp" "$native/source/game_morgan/CStartGameScreen.cpp" "$native/source/game_morgan/CLoadingScreen.cpp" "$hooks/CPatches.cpp" "$hooks/CUvAnim.cpp" "$hooks/Thread/CFastLoad.cpp" "$memory/CMemory.cpp" \
    "$fileio/FileIO.cpp" "$storage/PosixFileIO.cpp" "$storage/CStorageZip.cpp" "$storage/CStorageDisk.cpp" \
    "$fileio/HookAdapter.cpp" "$hooks/CHooks.cpp" "$fileio/FileOpenDiagnostics.cpp" "$memory/CHook.cpp" \
    "$multiplayer/Texts/CDeathWindow.cpp" "$multiplayer/Texts/CChatBubble.cpp" "$multiplayer/Texts/CTextLabel.cpp" "$multiplayer/Pools/Process/CTextLabelPool.cpp" "$multiplayer/Texts/CPlayerTags.cpp" "$multiplayer/Texts/CDebugInfo.cpp" "$multiplayer/Texts/CGameText.cpp" "$native/source/game_morgan/CFont.cpp" "$multiplayer/Guis/CGui.cpp" "$multiplayer/Guis/Font/CFontRender.cpp" \
    "$multiplayer/Guis/CSpawnScreen.cpp" "$multiplayer/Guis/CDialog.cpp" "$multiplayer/Guis/CButton.cpp" "$multiplayer/Guis/CXSing.cpp" "$multiplayer/Guis/CScoreBoard.cpp" "$multiplayer/Guis/CInterface.cpp" \
    "$hooks/MultiTouch/CTouchEvent.cpp" "$hooks/CWidgets.cpp" "$hooks/CCrashFix.cpp" \
    "$multiplayer/Guis/CChat.cpp" "$multiplayer/Guis/CCommand.cpp" "$multiplayer/Core/CMultiPlayerPacket.cpp" "$native/source/game_morgan/CUtil.cpp" "$native/source/game_morgan/TextureDatabaseRuntime.cpp" "$native/source/game_morgan/CTxdStore.cpp" "$native/source/game_morgan/CTxdManager.cpp" "$native/source/game_morgan/CWorld.cpp" \
    "$multiplayer/Core/CMultiPlayerNetwork.cpp" "$multiplayer/Core/CMultiPlayerOpcode.cpp" "$native/source/game_morgan/CEntryExitManager.cpp" \
    "$multiplayer/Pools/CNetGamePool.cpp" "$multiplayer/Pools/Process/CPlayerPool.cpp" "$multiplayer/Syncs/CRemotePlayer.cpp" \
    "$multiplayer/Syncs/Core/CMarkerIcon.cpp" "$multiplayer/Syncs/Core/CCheckPoint.cpp" "$multiplayer/Pools/Process/CMapIconPool.cpp" \
    "$multiplayer/Guis/Gps/CGPSMarker.cpp" "$multiplayer/Guis/Gps/CGPSNavigator.cpp" "$native/source/game_morgan/CSprite.cpp" \
    "$multiplayer/Snapshot/CModelSnapshot.cpp" \
    "$multiplayer/Syncs/CLocalPlayer.cpp" \
    "$multiplayer/Audio/CAudio.cpp" \
    "$multiplayer/Audio/Stream/CAudioStream.cpp" \
    "$native/source/game_morgan/Audio/CAEAudioHardware.cpp" "$native/source/game_morgan/Audio/CAudioEngine.cpp" \
    "$native/source/game_morgan/Audio/CBass.cpp" \
    "$imgui/imgui_impl_renderware.cpp"; do
    object="${source##*/}"
    if ! xcrun --sdk iphoneos clang++ -target arm64-apple-ios16.0 -isysroot "$sdk" \
        -std=c++17 "${fileio_flags[@]}" -isystem "$native/source/vendor" -Wall -Wextra -Werror -ferror-limit=0 -fstack-protector-strong \
        -fvisibility=hidden -fvisibility-inlines-hidden \
        -c "$source" -o "$out/fileio-objects/${object%.cpp}.o"; then
        compile_failed=1
    fi
done
if [[ "$compile_failed" -ne 0 ]]; then
    echo "Native C++ compilation failed; see all diagnostics above." >&2
    exit 1
fi
for unit in imgui imgui_draw; do
    xcrun --sdk iphoneos clang++ -target arm64-apple-ios16.0 -isysroot "$sdk" \
        -std=c++17 "${fileio_flags[@]}" -fvisibility=hidden -fvisibility-inlines-hidden \
        -c "$imgui/$unit.cpp" -o "$out/fileio-objects/$unit.o"
done
xcrun --sdk iphoneos clang++ -target arm64-apple-ios16.0 -isysroot "$sdk" \
    -std=c++17 "${fileio_flags[@]}" -Wall -Wextra -Werror -fstack-protector-strong \
        -fvisibility=hidden -fvisibility-inlines-hidden -fobjc-arc \
    -c "$storage/CStorage.mm" -o "$out/fileio-objects/CStorage.o"
for source in MGLauncherViewController MGLauncherRuntime MGStartupTickGate MGStartupKeychain MGFlutter MGGyro; do
    xcrun --sdk iphoneos clang++ -target arm64-apple-ios16.0 -isysroot "$sdk" \
        -std=c++17 "${launcher_flags[@]}" -fobjc-arc -fvisibility=hidden -fvisibility-inlines-hidden \
        -c "$launcher/$source.mm" -o "$out/fileio-objects/$source.o"
done
raknet="$native/source/vendor/raknet"
raknet_objects=()
for unit in BitStream CheckSum DataBlockEncryptor DataCompressor DS_HuffmanEncodingTree GetTime InternalPacketPool NetworkTypes PluginInterface RakClient RakNetStatistics RakNetworkFactory RakPeer rakserver RakSleep Rand ReliabilityLayer rijndael RPCMap SHA1 SimpleMutex SocketLayer StringCompressor StringTable SAMP/samp_netencr SAMP/SAMPRPC SAMP/samp_auth; do
    object="$out/fileio-objects/raknet-${unit##*/}.o"
    xcrun --sdk iphoneos clang++ -target arm64-apple-ios16.0 -isysroot "$sdk" \
        -std=c++17 "${fileio_flags[@]}" -isystem "$native/source/vendor" -fvisibility=hidden -DRAKSAMP_CLIENT -DNDEBUG \
        -c "$raknet/$unit.cpp" -o "$object"
    raknet_objects+=("$object")
done

xcrun --sdk iphoneos clang++ -target arm64-apple-ios16.0 -isysroot "$sdk" -dynamiclib \
    "-Wl,-install_name,$install_name" -Wl,-undefined,error ${link_flags[@]+"${link_flags[@]}"} \
    "$out/fileio-objects/MGLauncherViewController.o" "$out/fileio-objects/MGLauncherRuntime.o" \
    "$out/fileio-objects/MGStartupKeychain.o" "$out/fileio-objects/MGStartupTickGate.o" "$out/fileio-objects/CMainMenuScreen.o" "$out/fileio-objects/CStartGameScreen.o" "$out/fileio-objects/CLoadingScreen.o" "$out/fileio-objects/MGFlutter.o" \
    "$out/fileio-objects/CStreaming.o" "$out/fileio-objects/CClock.o" "$out/fileio-objects/CTimer.o" "$out/fileio-objects/COptimizeDistance.o" "$out/fileio-objects/CGame.o" "$out/fileio-objects/CModelInfo.o" "$out/fileio-objects/CPools.o" "$out/fileio-objects/CSetting.o" "$out/fileio-objects/CServer.o" "$out/fileio-objects/CNetGame.o" "$out/fileio-objects/CTheZones.o" "$out/fileio-objects/CWeather.o" "$out/fileio-objects/CCamera.o" "$out/fileio-objects/CPad.o" "${raknet_objects[@]}" \
    "$out/fileio-objects/CdStreamInfo.o" \
    "$out/fileio-objects/CCommon.o" "$out/fileio-objects/CEntity.o" "$out/fileio-objects/CPhysical.o" "$out/fileio-objects/CStats.o" "$out/fileio-objects/CWeapon.o" \
    "$out/fileio-objects/CWidgetPlayerInfo.o" "$out/fileio-objects/CSprite2d.o" "$out/fileio-objects/CRGBA.o" "$out/fileio-objects/CHud.o" \
    "$out/fileio-objects/CPedIntelligence.o" "$out/fileio-objects/CTaskManager.o" "$out/fileio-objects/CTask.o" "$out/fileio-objects/CTaskSimpleJetPack.o" "$out/fileio-objects/CTaskSimplePlayerOnFoot.o" \
    "$out/fileio-objects/CAnimManager.o" "$out/fileio-objects/CAnimation.o" "$out/fileio-objects/CKeyGen.o" "$out/fileio-objects/CActor.o" "$out/fileio-objects/CActorPool.o" "$out/fileio-objects/CTextDraw.o" "$out/fileio-objects/CTextDrawPool.o" \
    "$out/fileio-objects/CRadar.o" "$out/fileio-objects/CGangZone.o" "$out/fileio-objects/CGangZonePool.o" \
    "$out/fileio-objects/CPickup.o" "$out/fileio-objects/CPickupPool.o" \
    "$out/fileio-objects/CPed.o" "$out/fileio-objects/CPlayerPed.o" "$out/fileio-objects/CPlayerPedSlot.o" "$out/fileio-objects/CPedPointerRecorder.o" \
    "$out/fileio-objects/CCameraAim.o" "$out/fileio-objects/CAspectRatio.o" "$out/fileio-objects/CCameraController.o" "$out/fileio-objects/CDraw.o" "$out/fileio-objects/CSensitivity.o" "$out/fileio-objects/CGyro.o" "$out/fileio-objects/MGGyro.o" \
    "$out/fileio-objects/CTouchInterface.o" \
    "$out/fileio-objects/CRealTimeShadow.o" "$out/fileio-objects/CRealTimeShadowManager.o" "$out/fileio-objects/CShadowCamera.o" \
    "$out/fileio-objects/CScripting.o" "$out/fileio-objects/CRunningScript.o" "$out/fileio-objects/CExceptions.o" \
    "$out/fileio-objects/CQuaternion.o" \
    "$out/fileio-objects/CTiltedRadar.o" "$out/fileio-objects/CTiltedCameraController.o" "$out/fileio-objects/CTiltedRadarGeometry.o" "$out/fileio-objects/CTiltedRadarMath.o" \
    "$out/fileio-objects/CDeathWindow.o" "$out/fileio-objects/CChatBubble.o" "$out/fileio-objects/CTextLabel.o" "$out/fileio-objects/CTextLabelPool.o" "$out/fileio-objects/CPlayerTags.o" "$out/fileio-objects/CDebugInfo.o" "$out/fileio-objects/CGameText.o" "$out/fileio-objects/CFont.o" "$out/fileio-objects/CGui.o" "$out/fileio-objects/CFontRender.o" \
    "$out/fileio-objects/CSpawnScreen.o" "$out/fileio-objects/CDialog.o" "$out/fileio-objects/CButton.o" "$out/fileio-objects/CXSing.o" "$out/fileio-objects/CScoreBoard.o" "$out/fileio-objects/CInterface.o" \
    "$out/fileio-objects/CTouchEvent.o" "$out/fileio-objects/CWidgets.o" "$out/fileio-objects/CCrashFix.o" \
    "$out/fileio-objects/CChat.o" "$out/fileio-objects/CCommand.o" "$out/fileio-objects/CMultiPlayerPacket.o" "$out/fileio-objects/CUtil.o" "$out/fileio-objects/TextureDatabaseRuntime.o" "$out/fileio-objects/CTxdStore.o" "$out/fileio-objects/CTxdManager.o" "$out/fileio-objects/CWorld.o" \
    "$out/fileio-objects/CMultiPlayerNetwork.o" "$out/fileio-objects/CMultiPlayerOpcode.o" "$out/fileio-objects/CEntryExitManager.o" \
    "$out/fileio-objects/CNetGamePool.o" "$out/fileio-objects/CPlayerPool.o" "$out/fileio-objects/CRemotePlayer.o" \
    "$out/fileio-objects/CMarkerIcon.o" "$out/fileio-objects/CCheckPoint.o" "$out/fileio-objects/CMapIconPool.o" \
    "$out/fileio-objects/CGPSMarker.o" "$out/fileio-objects/CGPSNavigator.o" "$out/fileio-objects/CSprite.o" \
    "$out/fileio-objects/CModelSnapshot.o" \
    "$out/fileio-objects/CLocalPlayer.o" \
    "$out/fileio-objects/CAudio.o" \
    "$out/fileio-objects/CAudioStream.o" \
    "$out/fileio-objects/CAEAudioHardware.o" "$out/fileio-objects/CAudioEngine.o" \
    "$out/fileio-objects/CBass.o" \
    "$out/fileio-objects/imgui.o" "$out/fileio-objects/imgui_draw.o" "$out/fileio-objects/imgui_impl_renderware.o" \
    "$out/fileio-objects/Main.o" "$out/fileio-objects/RenderWare.o" "$out/fileio-objects/CMultiPlayer.o" "$out/fileio-objects/CNecessary.o" \
    "$out/fileio-objects/CPatches.o" "$out/fileio-objects/CUvAnim.o" "$out/fileio-objects/CFastLoad.o" "$out/fileio-objects/CMemory.o" \
    "$out/fileio-objects/FileIO.o" "$out/fileio-objects/PosixFileIO.o" \
    "$out/fileio-objects/HookAdapter.o" "$out/fileio-objects/CStorage.o" \
    "$out/fileio-objects/CStorageZip.o" "$out/fileio-objects/CStorageDisk.o" \
    "$out/fileio-objects/FileOpenDiagnostics.o" "$out/fileio-objects/CHooks.o" "$out/fileio-objects/CHook.o" \
    -Xlinker -exported_symbols_list -Xlinker "$fileio/LauncherExports.txt" \
    -framework UIKit -framework Foundation -framework CoreFoundation -framework CoreGraphics -framework QuartzCore -framework UniformTypeIdentifiers -framework Security -framework CoreMotion -lz -lobjc -o "$dylib"

[[ "$(xcrun lipo -archs "$dylib")" == arm64 ]]
xcrun otool -hv "$dylib" | tee "$out/macho-header.txt"
awk '{ for (i=1;i<=NF;i++) if ($i == "DYLIB" || $i == "MH_DYLIB") found=1 } END { exit !found }' \
    "$out/macho-header.txt"
[[ "$(xcrun otool -D "$dylib" | sed -n '2p')" == "$install_name" ]]
xcrun otool -l "$dylib" > "$out/load-commands.txt"
awk '
    $1 == "cmd" { in_build_version = ($2 == "LC_BUILD_VERSION") }
    in_build_version && $1 == "platform" && ($2 == "2" || toupper($2) == "IOS") { ios=1 }
    $1 == "sectname" { in_initializer_section = ($2 == "__mod_init_func" || $2 == "__init_offsets") }
    in_initializer_section && $1 == "size" && $2 !~ /^0(x0+)?$/ { module_init=1 }
    END { exit (!ios || !module_init) }
' "$out/load-commands.txt"

xcrun nm -gjU "$dylib" > "$out/exports.txt"
LC_ALL=C sort -u "$fileio/LauncherExports.txt" > "$out/expected-exports.txt"
LC_ALL=C sort -u "$out/exports.txt" > "$out/actual-exports.txt"
diff -u "$out/expected-exports.txt" "$out/actual-exports.txt"
grep -Fxq '_MGOpenGameReadStream' "$out/exports.txt"
grep -Fxq '_OBJC_CLASS_$_MGLauncherViewController' "$out/exports.txt"
grep -Fxq '_OBJC_CLASS_$_MGLauncherPresentationCoordinator' "$out/exports.txt"
symbols="$out/fileio-objects/symbols-before-strip.txt"
xcrun nm -j "$dylib" > "$symbols"
grep -Fxq '_MGInstallLauncherRuntime' "$symbols"
grep -Fxq '__ZN6CClock11SetGameTimeEii' "$symbols"
grep -Fxq '__ZN5CGame17GetActiveInteriorEv' "$symbols"
# Require each edge in the native startup chain before Release strips local symbols.
startup_symbols="$out/startup-calls.txt"
: > "$startup_symbols"
check_call() {
    local caller="$1" callee="$2" imports
    imports="$(xcrun nm -u -j "$out/fileio-objects/$caller.o")"
    if ! grep -Fq "$callee" <<< "$imports"; then
        printf 'Missing required call from %s to %s\n' "$caller" "$callee" >&2
        return 1
    fi
    printf '%s -> %s\n' "$caller" "$callee" >> "$startup_symbols"
}
check_call MGLauncherRuntime 'InitialiseClient'
check_call Main 'InitialiseRenderWare'
check_call Main 'CSettingC1'
check_call CMultiPlayer 'CServer12SetIpConnect'
check_call CMultiPlayer 'CNetGameC1'
check_call CMultiPlayer 'CNetGame7Process'
check_call CNetGame 'RakNetworkFactory21GetRakClientInterface'
check_call CNetGame 'CNetGamePoolC1'
check_call CNetGamePool 'CPlayerPoolC1'
check_call CPlayerPool 'CLocalPlayerC1'
check_call CPlayerPool 'CLocalPlayer7Process'
check_call CLocalPlayer 'CPlayerPedC1'
check_call CNetGame 'CMultiPlayerNetwork6Update'
check_call raknet-RakPeer 'CMultiPlayerOpcode7Receive'
check_call CMultiPlayerOpcode 'CEntryExitManager25DisableInteriorEnterExits'
check_call CNetGame 'SetGameWeather'
check_call CNetGame 'DisplayHUD'
check_call CNetGame 'DisplayWidgets'
check_call CNetGame 'm_pGameCamera'
check_call CNetGame 'SetPosition'
check_call CNetGame 'LookAtPoint'
check_call CMultiPlayer 'CScripting4Init'
check_call CNetGame 'CClock15EnableGameClock'
check_call CClock 'CGame17GetActiveInterior'
check_call CGame 'CScripting13ScriptCommand'
check_call CHooks 'CMultiPlayer7Process'
check_call CHooks 'CTiltedRadar10Initialise'
check_call CHooks 'CWorld10Initialise'
check_call CHooks 'CStreaming10Initialise'
check_call CHooks 'CPhysical10Initialise'
check_call CHooks 'CStats10Initialise'
check_call CHooks 'CWidgetPlayerInfo10Initialise'
check_call CWidgetPlayerInfo 'CSprite2d12DrawBarChart'
check_call CHooks 'CAspectRatio10Initialise'
check_call CHooks 'CDraw10Initialise'
check_call CHooks 'CCameraController15AimForTheCamera'
check_call CDraw 'CCameraController17IsCameraAllAiming'
check_call CMultiPlayer 'CCameraController10Initialise'
for setter in SetOnfootTouch SetAimTouch SetIncarTouch; do
    check_call CMultiPlayer "$setter"
    check_call CCommand "$setter"
done
check_call CMultiPlayer 'CDraw6SetFov'
check_call CCommand 'CDraw6SetFov'
check_call CMultiPlayer 'CCameraAim10Initialise'
check_call CMultiPlayer 'CAspectRatio7SetSize'
check_call CCommand 'CAspectRatio7SetSize'
check_call CStreaming 'CdStreamInfo12CdStreamOpen'
check_call CPatches 'CUvAnim10Initialise'
check_call CMultiPlayer 'COptimizeDistance23OptimizeStreamingMemory'
check_call CMultiPlayer 'CGame13MenuSwitchOff'
check_call CMultiPlayer 'CGame17SetPlayerMaxStats'
check_call CScripting 'CRunningScript17ProcessOneCommand'
check_call Main 'CGame10Initialize'
check_call Main 'CGame12Uninitialise'
check_call Main 'CMultiPlayer10Initialise'
check_call RenderWare 'CMemory20HasMappedInstruction'
check_call CMultiPlayer 'CPatches10SetPatches'
check_call CMultiPlayer 'CHooks10SetHooking'
check_call CMultiPlayer 'CGame12SetFramerate'
check_call CGame 'MGSetGtaFramerate'
check_call CPatches 'CMemory18WriteProcessMemory'
check_call CPatches 'CMemory7makeRET'
check_call CPatches 'CFastLoad10Initialise'
check_call CFastLoad 'CMemory7makeRET'
check_call CHooks 'CHook9GetThread'
check_call CHooks 'CHook15InstallFileOpen'
check_call MGStartupTickGate 'CMainMenuScreen6Update'
check_call CMainMenuScreen 'CMultiPlayer9StartSAMP'
check_call CMainMenuScreen 'ExitGame'
check_call MGLauncherRuntime 'UninitialiseClient'
check_call MGLauncherRuntime '_exit'
check_call CMultiPlayer 'CStartGameScreen14OnNewGameCheck'
cat "$startup_symbols"
echo 'ARITA ONLOAD CALL CHAIN PASS'
# Internal C++ names may be inlined or dead-stripped in Release.
if [[ "$configuration" == Debug ]]; then
    grep -Fq '6morgan6fileio11HookAdapter' "$symbols"
    grep -Fq '6morgan6fileio8CStorage18OpenBundleResource' "$symbols"
    grep -Fxq '__ZN6CHooks16InstallNecessaryEv' "$symbols"
    grep -Fxq '__ZN8CPatches22InstallLoadFilePatchesEv' "$symbols"
fi
xcrun nm -um "$dylib" > "$out/undefined-symbols.txt"
xcrun otool -L "$dylib" > "$out/dependencies.txt"
grep -Fq '/System/Library/Frameworks/UIKit.framework/UIKit ' "$out/dependencies.txt"
grep -Fq '/System/Library/Frameworks/UniformTypeIdentifiers.framework/UniformTypeIdentifiers ' "$out/dependencies.txt"
grep -Fq '/usr/lib/libz.1.dylib ' "$out/dependencies.txt"
awk '
    NR <= 2 { next }
    $1 !~ /^\/System\/Library\/Frameworks\// && $1 !~ /^\/usr\/lib\// { unexpected=1 }
    END { exit unexpected }
' "$out/dependencies.txt"
if ! codesign --display --verbose=4 "$dylib" > "$out/signature-metadata.txt" 2>&1; then
    echo 'No readable code signature. This is a compile artifact.' >> "$out/signature-metadata.txt"
fi
if [[ "$configuration" == Release ]]; then
    xcrun dsymutil "$dylib" -o "$dsym"
    dwarf="$dsym/Contents/Resources/DWARF/MorganLauncher"
    [[ -s "$dwarf" ]]
    xcrun otool -l "$dwarf" > "$out/dsym-load-commands.txt"
    grep -Eq '^[[:space:]]*sectname __debug_info$' "$out/dsym-load-commands.txt"
    if grep -Eq '^[[:space:]]*segname __DWARF$' "$out/load-commands.txt"; then
        echo 'Release framework contains embedded DWARF data.' >&2
        exit 1
    fi
    cp "$out/actual-exports.txt" "$out/exports-before-strip.txt"
    xcrun dyld_info -exports "$dylib" > "$out/export-trie-before-strip.txt"
    read -r init_segment init_section <<< "$(awk '
        $1 == "sectname" && ($2 == "__mod_init_func" || $2 == "__init_offsets") { section=$2 }
        section && $1 == "segname" { print $2, section; exit }
    ' "$out/load-commands.txt")"
    xcrun otool -s "$init_segment" "$init_section" "$dylib" > "$out/initializer-before-strip.txt"
    initializer_address="$(xcrun nm -n "$dylib" | awk '$NF == "_MGInstallLauncherRuntime" { print "0x" $1 }')"
    [[ -n "$initializer_address" ]]
    xcrun atos -arch arm64 -o "$dwarf" "$initializer_address" | tee "$out/symbolication.txt"
    grep -Fq 'MGInstallLauncherRuntime' "$out/symbolication.txt"

    xcrun strip -x "$dylib"
    xcrun nm -gjU "$dylib" | LC_ALL=C sort -u > "$out/exports-after-strip.txt"
    diff -u "$out/expected-exports.txt" "$out/exports-after-strip.txt"
    xcrun dyld_info -exports "$dylib" > "$out/export-trie-after-strip.txt"
    diff -u "$out/export-trie-before-strip.txt" "$out/export-trie-after-strip.txt"
    grep -Fq '_MGOpenGameReadStream' "$out/export-trie-after-strip.txt"
    xcrun otool -s "$init_segment" "$init_section" "$dylib" > "$out/initializer-after-strip.txt"
    diff -u "$out/initializer-before-strip.txt" "$out/initializer-after-strip.txt"
    xcrun nm -j "$dylib" > "$out/fileio-objects/symbols-after-strip.txt"
    if grep -Fxq '_MGInstallLauncherRuntime' "$out/fileio-objects/symbols-after-strip.txt"; then
        echo 'Release framework still contains the local initializer symbol.' >&2
        exit 1
    fi
    echo 'RELEASE STRIP PASS: export trie and initializer payload retained.'
    binary_uuid="$(xcrun dwarfdump --uuid "$dylib" | awk '$1 == "UUID:" { print $2, $3 }')"
    dsym_uuid="$(xcrun dwarfdump --uuid "$dsym" | awk '$1 == "UUID:" { print $2, $3 }')"
    [[ -n "$binary_uuid" && "$binary_uuid" == "$dsym_uuid" ]]
    printf 'Framework UUID %s\n' "$binary_uuid" > "$out/dsym-uuid.txt"
    echo 'DEBUG SYMBOL SEPARATION PASS: matching Release framework and dSYM UUID.'
fi
(cd "$out" && shasum -a 256 MorganLauncher.framework/MorganLauncher MorganLauncher.framework/Info.plist) | tee "$out/SHA256SUMS"
echo 'FRAMEWORK BUILD PASS: iPhoneOS ARM64 MH_DYLIB, install name and dependencies verified.'
echo 'INITIALIZER CHECK PASS: module initializer and launcher coordinator symbols are present.'

mkdir "$icon"
xcrun --sdk iphoneos actool --compile "$icon" --platform iphoneos \
    --minimum-deployment-target 16.0 --target-device iphone \
    --app-icon MorganAppIcon --output-partial-info-plist "$icon/partial-info.plist" \
    "$project/Resources/MorganAssets.xcassets"
[[ -s "$icon/Assets.car" ]]
[[ "$(plutil -extract CFBundleIcons.CFBundlePrimaryIcon.CFBundleIconName raw -o - \
    "$icon/partial-info.plist")" == MorganAppIcon ]]
echo 'ICON CATALOG BUILD PASS: isolated output and primary icon name verified.'
