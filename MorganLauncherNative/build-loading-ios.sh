#!/usr/bin/env bash
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
module="${2:-$repo/MorganFlutter}"
out="${1:?Usage: build-loading-ios.sh <artifact-dir> [module-dir]}"
mkdir -p "$out"
out="$(cd "$out" && pwd)"
engine=55eae6864b296dd9f43b2cc7577ec256e5c32a8d

flutter --version
python3 -c '
from pathlib import Path
import json, sys
sdk = Path(sys.argv[2]).resolve().parents[1]
version = json.loads((sdk / "bin/cache/flutter.version.json").read_text())
assert version["frameworkVersion"] == "3.22.1", version
assert version["engineRevision"] == sys.argv[1], version
' "$engine" "$(command -v flutter)"

cd "$module"
flutter pub get --enforce-lockfile
flutter analyze --no-pub
flutter test --no-pub
build="$(mktemp -d)"
trap 'rm -rf "$build"' EXIT
flutter build ios-framework --no-debug --no-profile --no-plugins --no-pub --output="$build"

source="$build/Release/App.xcframework/ios-arm64/App.framework"
target="$out/MorganFlutter.framework"
[[ -d "$source" && ! -e "$target" ]]
ditto --norsrc --noextattr "$source" "$target"
mv "$target/App" "$target/MorganFlutter"
# Preserve the original __LINKEDIT layout while changing the install name.
xcrun install_name_tool -id @rpath/MorganFlutter.framework/MorganFlutter "$target/MorganFlutter"
if codesign -dv "$target/MorganFlutter" >/dev/null 2>&1; then
    codesign --remove-signature "$target/MorganFlutter"
fi
[[ "$(xcrun otool -D "$target/MorganFlutter" | sed -n '2p')" == @rpath/MorganFlutter.framework/MorganFlutter ]]
plutil -replace CFBundleExecutable -string MorganFlutter "$target/Info.plist"
plutil -replace CFBundleIdentifier -string com.morgangame.ios.flutter "$target/Info.plist"
plutil -replace CFBundleName -string MorganFlutter "$target/Info.plist"
plutil -insert MorganFlutterEngine -string "$engine" "$target/Info.plist"
plutil -lint "$target/Info.plist"

[[ "$(xcrun lipo -archs "$target/MorganFlutter")" == arm64 ]]
xcrun nm -gjU "$target/MorganFlutter" > "$out/aot-symbols.txt"
grep -Fxq _kDartIsolateSnapshotData "$out/aot-symbols.txt"
grep -Fxq _kDartIsolateSnapshotInstructions "$out/aot-symbols.txt"
test -f "$target/flutter_assets/AssetManifest.bin"
test -f "$target/flutter_assets/assets/backgroud01.png"
for font in Kanit-Bold.ttf Kanit-Black.ttf NotoSans-Regular.ttf Rajdhani-Medium.ttf; do
    test -f "$target/flutter_assets/assets/fonts/$font"
done
(cd "$out" && shasum -a 256 MorganFlutter.framework/MorganFlutter MorganFlutter.framework/Info.plist > SHA256SUMS)
COPYFILE_DISABLE=1 tar -C "$out" -cf "$out/MorganFlutter.framework.tar" MorganFlutter.framework
