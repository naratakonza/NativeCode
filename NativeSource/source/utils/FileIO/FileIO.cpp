#include "FileIO.h"

#include <algorithm>
#include <cstring>

namespace morgan::fileio {
namespace {

struct Rule {
    std::string_view request;
    Resource resource;
    const char* leaf;
    std::string_view prefixedRequest = {};
};

constexpr Rule Rules[] = {
    {"mainv1.scm", Resource::MainScript, "main.scm", "data/script/mainv1.scm"},
    {"scriptv1.img", Resource::ScriptImage, "script.img", "data/script/scriptv1.img"},
};

char Fold(char c) noexcept {
    if (c == '\\') return '/';
    if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');
    return c;
}

bool MatchesFolded(std::string_view path, std::string_view rule) noexcept {
    if (path.size() != rule.size()) return false;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (Fold(path[i]) != Fold(rule[i])) return false;
    }
    return true;
}

PathRejection InputRejection(std::string_view path) noexcept {
    if (path.empty()) return PathRejection::Empty;
    if (path.size() > MaxPathBytes) return PathRejection::Length;
    for (std::size_t i = 0; i < path.size();) {
        const auto lead = static_cast<unsigned char>(path[i++]);
        if (lead < 0x20 || lead == 0x7f) return PathRejection::UnsafeCharacter;
        if (lead < 0x80) continue;
        if (lead < 0xc2 || lead > 0xf4) return PathRejection::UnsafeCharacter;
        const std::size_t extra = lead < 0xe0 ? 1 : lead < 0xf0 ? 2 : 3;
        if (extra > path.size() - i) return PathRejection::UnsafeCharacter;
        const auto second = static_cast<unsigned char>(path[i]);
        if ((lead == 0xe0 && second < 0xa0) || (lead == 0xed && second >= 0xa0) ||
            (lead == 0xf0 && second < 0x90) || (lead == 0xf4 && second >= 0x90)) return PathRejection::UnsafeCharacter;
        for (std::size_t j = 0; j < extra; ++j) {
            if ((static_cast<unsigned char>(path[i++]) & 0xc0) != 0x80) return PathRejection::UnsafeCharacter;
        }
    }
    return PathRejection::None;
}

Resolution RejectedPath(PathRejection reason) noexcept {
    Resolution result{Disposition::Reject};
    result.rejection = reason;
    return result;
}

Resolution ResolveAlias(std::string_view path) noexcept {
    if (path.find(':') != std::string_view::npos) return RejectedPath(PathRejection::UnsafeCharacter);
    for (std::size_t start = 0;;) {
        const auto end = path.find_first_of("/\\", start);
        const auto part = path.substr(start, end == std::string_view::npos ? end : end - start);
        if (part.empty()) return RejectedPath(PathRejection::EmptyComponent);
        if (part == "." || part == "..") return RejectedPath(PathRejection::Traversal);
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    for (const auto& rule : Rules) {
        if (MatchesFolded(path, rule.request) ||
            (!rule.prefixedRequest.empty() && MatchesFolded(path, rule.prefixedRequest))) {
            return {Disposition::Redirect, rule.resource, 0};
        }
    }
    return {Disposition::Original};
}

thread_local bool opening = false;

struct OpenGuard {
    OpenGuard() noexcept { opening = true; }
    ~OpenGuard() { opening = false; }
};

void Emit(DebugDiagnostics diagnostics, Diagnostic event) noexcept {
#ifndef NDEBUG
    if (diagnostics.emit) diagnostics.emit(diagnostics.context, event);
#else
    (void)diagnostics;
    (void)event;
#endif
}

OpenResult Invalid(DebugDiagnostics diagnostics) noexcept {
    Emit(diagnostics, Diagnostic::InvalidInput);
    return {{}, std::make_error_code(std::errc::invalid_argument)};
}

} // namespace

Resolution ResolveSampPath(std::string_view path) noexcept {
    if (const auto reason = InputRejection(path); reason != PathRejection::None) return RejectedPath(reason);
    return ResolveAlias(path);
}

namespace storage {

bool IsSampStatePath(std::string_view path) noexcept
{
    const auto separator = path.find_first_of("/\\");
    if (separator == std::string_view::npos || !MatchesFolded(path.substr(0, separator), GroupNames[0])) return false;
    path.remove_prefix(separator + 1);
    const auto leaf = path.substr(0, path.find_first_of("/\\"));
    return MatchesFolded(leaf, SampLogName) || MatchesFolded(leaf, SampSettingsName);
}

static PathRejection AssetPathRejection(std::string_view path) noexcept {
    if (IsSampStatePath(path)) return PathRejection::ReservedComponent;
    if (path.empty()) return PathRejection::Empty;
    if (path.size() > MaxAssetPathBytes) return PathRejection::Length;
    std::size_t depth = 0;
    for (;;) {
        const auto slash = path.find('/');
        const auto part = path.substr(0, slash);
        if (part.empty()) return PathRejection::EmptyComponent;
        if (part == "." || part == "..") return PathRejection::Traversal;
        if (part.front() == '.' || part.back() == '.') return PathRejection::InvalidNormalization;
        if (part.size() > 100) return PathRejection::Length;
        if (++depth > 12) return PathRejection::Depth;
        for (std::size_t i = 0; i < part.size(); ++i) {
            const char c = Fold(part[i]);
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') continue;
            if (c == ' ' && slash == std::string_view::npos && part.find_first_not_of(' ', i) == std::string_view::npos) {
                return PathRejection::TrailingSpace;
            }
            return PathRejection::UnsafeCharacter;
        }
        for (const auto reserved : {"frameworks", "_codesignature", "__macosx"}) {
            if (MatchesFolded(part, reserved)) return PathRejection::ReservedComponent;
        }
        for (const std::string_view suffix : {".framework", ".bundle", ".app", ".appex", ".dylib"}) {
            if (part.size() >= suffix.size() && MatchesFolded(part.substr(part.size() - suffix.size()), suffix)) {
                return PathRejection::ReservedComponent;
            }
        }
        if (slash == std::string_view::npos) return PathRejection::None;
        path.remove_prefix(slash + 1);
    }
}

bool SafeAssetPath(std::string_view path) noexcept {
    return AssetPathRejection(path) == PathRejection::None;
}

bool IsRootDataConfig(std::string_view path) noexcept {
    return std::find(RootDataConfigs.begin(), RootDataConfigs.end(), path) != RootDataConfigs.end();
}

static PathRejection GroupPathRejection(std::string_view path, std::size_t group) noexcept {
    const auto reason = AssetPathRejection(path);
    if (reason != PathRejection::None) return reason;
    const auto slash = path.find('/');
    if (slash == std::string_view::npos && group == DataGroup && IsRootDataConfig(path)) return PathRejection::None;
    return group < GroupCount && slash != std::string_view::npos && path.substr(0, slash) == GroupNames[group]
        ? PathRejection::None : PathRejection::InvalidNormalization;
}

bool AllowedAssetPath(std::string_view path, std::size_t group) noexcept {
    return GroupPathRejection(path, group) == PathRejection::None;
}
} // namespace storage

namespace {

std::string_view StripRelativeMarker(std::string_view path) noexcept {
    if (path.size() > 2 && path[0] == '.' && Fold(path[1]) == '/') path.remove_prefix(2);
    return path;
}

Resolution ResolveRelativeAssetPath(std::string_view path) noexcept {
    if (const auto reason = InputRejection(path); reason != PathRejection::None) return RejectedPath(reason);
    auto result = ResolveAlias(path);
    if (result.disposition != Disposition::Original) return result;
    const auto slash = path.find_first_of("/\\");
    if (slash == std::string_view::npos) {
        for (const std::string_view config : storage::RootDataConfigs) {
            if (!MatchesFolded(path, config)) continue;
            result.group = storage::DataGroup;
            result.length = config.size();
            std::copy(config.begin(), config.end(), result.normalized.begin());
            result.disposition = Disposition::Redirect;
            return result;
        }
        return result;
    }
    for (std::size_t i = 0; i < storage::GroupCount; ++i) {
        const auto* root = storage::GroupNames[i];
        if (!MatchesFolded(path.substr(0, slash), root)) continue;
        result.group = i;
        if (path.size() > result.normalized.size()) {
            result.disposition = Disposition::Reject;
            result.rejection = PathRejection::Length;
            return result;
        }
        result.length = path.size();
        for (std::size_t j = 0; j < path.size(); ++j) result.normalized[j] = Fold(path[j]);
        std::copy_n(root, slash, result.normalized.data());
        result.rejection = storage::GroupPathRejection(result.Path(), i);
        if (result.rejection != PathRejection::None) {
            result.disposition = Disposition::Reject;
            return result;
        }
        result.disposition = Disposition::Redirect;
        // Direct SAMP requests use the same pinned source as the existing aliases.
        if (i == 0) {
            const auto leaf = result.Path().substr(slash + 1);
            for (const auto& rule : Rules) if (leaf == rule.leaf) result.resource = rule.resource;
        }
        return result;
    }
    return result;
}

// A case-sensitive match for the OS path. Only the game-relative suffix is folded.
std::string_view BundleSuffix(std::string_view path, std::string_view bundleRoot) noexcept {
    if (bundleRoot.empty() || bundleRoot.front() != '/' || path.size() > MaxPathBytes) return {};
    while (bundleRoot.size() > 1 && bundleRoot.back() == '/') bundleRoot.remove_suffix(1);
    if (bundleRoot.size() == 1 || path.size() <= bundleRoot.size() + 1 ||
        path.compare(0, bundleRoot.size(), bundleRoot) != 0 || Fold(path[bundleRoot.size()]) != '/') return {};
    return path.substr(bundleRoot.size() + 1);
}

AssetPathKind PathKind(std::string_view path) noexcept {
    if (path.empty()) return AssetPathKind::Empty;
    const auto slash = path.find_first_of("/\\");
    if (slash == 0) return AssetPathKind::Absolute;
    if (path.find(':') < slash) return AssetPathKind::DriveQualified;
    return slash == std::string_view::npos ? AssetPathKind::Rootless : AssetPathKind::Relative;
}

AssetPathIssue Issue(const Resolution& asset) noexcept {
    return asset.disposition == Disposition::Reject ? AssetPathIssue::InvalidPath : AssetPathIssue::None;
}

bool SameAsset(const Resolution& left, const Resolution& right) noexcept {
    if (left.resource != Resource::None || right.resource != Resource::None) return left.resource == right.resource;
    return left.group == right.group && left.Path() == right.Path();
}

bool IsComponentSuffix(std::string_view path, std::string_view suffix) noexcept {
    if (suffix.empty() || path.size() < suffix.size()) return false;
    const auto offset = path.size() - suffix.size();
    if (offset && Fold(path[offset - 1]) != '/') return false;
    return std::equal(suffix.begin(), suffix.end(), path.begin() + offset,
                      [](char left, char right) { return Fold(left) == Fold(right); });
}

} // namespace

Resolution ResolveGameAssetPath(std::string_view path) noexcept {
    if (const auto reason = InputRejection(path); reason != PathRejection::None) return RejectedPath(reason);
    const auto kind = PathKind(path);
    if (kind == AssetPathKind::Absolute || kind == AssetPathKind::DriveQualified) return {Disposition::Original};
    return ResolveRelativeAssetPath(StripRelativeMarker(path));
}

GameAssetPath ResolveGameOpenPath(std::string_view logical, std::string_view resolved,
                                  std::string_view bundleRoot) noexcept {
    const auto logicalSuffix = BundleSuffix(logical, bundleRoot);
    const auto primary = logicalSuffix.empty() ? ResolveGameAssetPath(logical) : ResolveRelativeAssetPath(logicalSuffix);
    auto kind = PathKind(logical);
    if (!logicalSuffix.empty()) kind = AssetPathKind::BundleAbsolute;
    else if (primary.group < storage::GroupCount) kind = AssetPathKind::Asset;
    GameAssetPath result{primary, kind, Issue(primary)};
    result.rejection = primary.rejection;
    if (primary.disposition == Disposition::Reject && primary.group >= storage::GroupCount) return result;

    const auto resolvedSuffix = BundleSuffix(resolved, bundleRoot);
    if (resolvedSuffix.empty()) {
        result.resolvedKind = resolved.empty() ? ResolvedPathKind::Unavailable :
            bundleRoot.empty() ? ResolvedPathKind::BundleRootUnavailable : ResolvedPathKind::OutsideBundle;
        return result;
    }
    result.bundleSuffixOffset = resolved.size() - resolvedSuffix.size();

    // The stock builder prepends the Bundle even to an absolute logical request.
    if (primary.disposition != Disposition::Reject && kind == AssetPathKind::BundleAbsolute &&
        primary.group < storage::GroupCount && resolvedSuffix.size() == logical.size() &&
        IsComponentSuffix(resolvedSuffix, logical)) {
        result.resolvedKind = ResolvedPathKind::BundleEcho;
        return result;
    }

    const auto secondary = ResolveRelativeAssetPath(resolvedSuffix);
    result.resolvedKind = secondary.disposition == Disposition::Reject ? ResolvedPathKind::BundleInvalid :
        secondary.group < storage::GroupCount ? ResolvedPathKind::BundleAsset : ResolvedPathKind::BundleNonAsset;
    if (primary.disposition == Disposition::Reject) {
        // GTA trims trailing whitespace on its copy. Accept only terminal ASCII
        // spaces on a relative asset and only when stock independently agrees.
        if (kind == AssetPathKind::Asset && primary.rejection == PathRejection::TrailingSpace &&
            secondary.disposition == Disposition::Redirect && secondary.group == primary.group &&
            !secondary.Path().empty()) {
            const auto end = logical.find_last_not_of(' ');
            const auto trimmed = ResolveGameAssetPath(logical.substr(0, end + 1));
            if (trimmed.disposition == Disposition::Redirect && trimmed.group == secondary.group &&
                trimmed.Path() == secondary.Path() && trimmed.resource == secondary.resource) {
                result.asset = trimmed;
                result.issue = AssetPathIssue::None;
                result.usedResolvedPath = true;
                result.stockTrailingSpace = true;
            }
        }
        return result;
    }
    if (secondary.disposition == Disposition::Reject) {
        result.asset = secondary;
        result.issue = Issue(secondary);
        result.rejection = secondary.rejection;
        result.usedResolvedPath = true;
        return result;
    }

    if (primary.group >= storage::GroupCount && secondary.group >= storage::GroupCount) return result;
    if (primary.group < storage::GroupCount) {
        if (SameAsset(primary, secondary)) return result;
    } else if ((kind == AssetPathKind::Relative || kind == AssetPathKind::Rootless) &&
               IsComponentSuffix(resolvedSuffix, StripRelativeMarker(logical))) {
        result.asset = secondary;
        result.usedResolvedPath = true;
        return result;
    }
    result.asset = RejectedPath(PathRejection::ConflictingPaths);
    result.issue = AssetPathIssue::ConflictingPaths;
    result.rejection = PathRejection::ConflictingPaths;
    return result;
}

void ApplySampAssetOverride(Resolution& asset) noexcept
{
    if (asset.disposition != Disposition::Redirect || asset.group != storage::DataGroup) return;

    struct Override {
        std::string_view original;
        std::string_view replacement;
    };
    static constexpr Override overrides[] = {
        {"data/peds.ide", "SAMP/peds.ide"},
        {"data/vehicles.ide", "SAMP/vehicles.ide"},
        {"data/gta.dat", "SAMP/gta.dat"},
        {"data/handling.cfg", "SAMP/handling.cfg"},
        {"data/paths/tracks2.dat", "SAMP/tracks2.dat"},
        {"data/paths/tracks4.dat", "SAMP/tracks4.dat"},
    };

    for (const auto& rule : overrides) {
        if (asset.Path() != rule.original) continue;
        std::copy(rule.replacement.begin(), rule.replacement.end(), asset.normalized.begin());
        asset.length = rule.replacement.size();
        asset.group = 0;
        return;
    }
}

bool HasSampTrackHeader(std::FILE* stream) noexcept
{
    char header[32];
    return std::fgets(header, sizeof(header), stream) && std::strchr(header, '\n') &&
           std::fseek(stream, 0, SEEK_SET) == 0;
}

const char* SampLeaf(Resource resource) noexcept {
    for (const auto& rule : Rules) {
        if (resource == rule.resource) return rule.leaf;
    }
    return nullptr;
}

OpenResult OpenFile(std::string_view path, Access access, OriginalOpen original,
                    ReplacementOpen replacement, Mode mode, DebugDiagnostics diagnostics) noexcept {
    if (opening) return {{}, std::make_error_code(std::errc::resource_deadlock_would_occur)};
    const OpenGuard guard;
    if (!original.call || InputRejection(path) != PathRejection::None) return Invalid(diagnostics);
    if (mode == Mode::OriginalOnly || access != Access::ReadOnly) {
        return original.call(original.context, path, access);
    }

    const auto resolution = ResolveGameAssetPath(path);
    if (resolution.disposition == Disposition::Reject) return Invalid(diagnostics);
    if (resolution.disposition == Disposition::Original) {
        return original.call(original.context, path, access);
    }

    if (!replacement.call) {
        if (mode == Mode::SampReadOnly) return original.call(original.context, path, access);
        return {{}, std::make_error_code(std::errc::operation_not_supported)};
    }
    auto result = replacement.call(replacement.context, resolution);
    if (result.file && !result.error) {
        Emit(diagnostics, Diagnostic::ReplacementOpened);
        return result;
    }
    if (mode == Mode::SampReadOnly && !result.file && result.error == std::errc::no_such_file_or_directory) {
        Emit(diagnostics, Diagnostic::OriginalFallback);
        return original.call(original.context, path, access);
    }
    Emit(diagnostics, Diagnostic::ReplacementDenied);
    return {{}, result.error ? result.error : std::make_error_code(std::errc::io_error)};
}

} // namespace morgan::fileio
