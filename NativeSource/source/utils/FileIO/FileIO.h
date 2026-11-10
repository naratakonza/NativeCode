#pragma once

#include <array>
#include <cstdio>
#include <memory>
#include <string_view>
#include <system_error>

namespace morgan::fileio {

inline constexpr std::size_t MaxPathBytes = 1024;

namespace storage {
inline constexpr std::array<const char*, 8> GroupNames{"SAMP", "anim", "audio", "data", "models", "texdb", "text", "textures"};
inline constexpr std::size_t GroupCount = GroupNames.size();
inline constexpr std::size_t DataGroup = 3;
inline constexpr std::array<const char*, 4> RootDataConfigs{
    "scache.txt", "scache_small.txt", "scache_small_low.txt", "stream.ini"
};
inline constexpr std::size_t MaxAssetPathBytes = 240;
inline constexpr char SampLogName[] = "Morgan.log";
inline constexpr char SampSettingsName[] = "settings.json";
inline constexpr std::size_t MaxSampSettingsBytes = 128 * 1024;
bool IsSampStatePath(std::string_view path) noexcept;
bool SafeAssetPath(std::string_view path) noexcept;
bool IsRootDataConfig(std::string_view path) noexcept;
bool AllowedAssetPath(std::string_view path, std::size_t group) noexcept;
}

enum class Resource { None, MainScript, ScriptImage };
enum class Disposition { Original, Redirect, Reject };
enum class AssetPathKind { Empty, Rootless, Relative, Asset, BundleAbsolute, Absolute, DriveQualified };
enum class AssetPathIssue { None, InvalidPath, ConflictingPaths };
enum class ResolvedPathKind { Unchecked, Unavailable, BundleRootUnavailable, OutsideBundle,
                              BundleNonAsset, BundleAsset, BundleInvalid, BundleEcho };
enum class PathRejection { None, Empty, Length, Depth, EmptyComponent, Traversal, UnsafeCharacter,
                           ReservedComponent, TrailingSpace, InvalidNormalization, ConflictingPaths };
enum class Mode { OriginalOnly, SampReadOnly, ExternalOnly };
enum class Access { ReadOnly, Other };
enum class Diagnostic { InvalidInput, OriginalFallback, ReplacementOpened, ReplacementDenied };

struct Resolution {
    Disposition disposition;
    Resource resource = Resource::None;
    std::size_t group = storage::GroupCount;
    std::array<char, storage::MaxAssetPathBytes> normalized{};
    std::size_t length = 0;
    PathRejection rejection = PathRejection::None;

    std::string_view Path() const noexcept { return {normalized.data(), length}; }
};

struct GameAssetPath {
    Resolution asset{Disposition::Original};
    AssetPathKind inputKind = AssetPathKind::Empty;
    AssetPathIssue issue = AssetPathIssue::None;
    bool usedResolvedPath = false;
    ResolvedPathKind resolvedKind = ResolvedPathKind::Unchecked;
    PathRejection rejection = PathRejection::None;
    std::size_t bundleSuffixOffset = 0; // Offset into the original resolved input, never an owned path.
    bool stockTrailingSpace = false;
};

// Input is a borrowed, bounded byte view. No caller pointer is dereferenced as a C string.
Resolution ResolveSampPath(std::string_view path) noexcept;
Resolution ResolveGameAssetPath(std::string_view path) noexcept;
// bundleRoot is the app's exact resource path obtained independently of the request.
GameAssetPath ResolveGameOpenPath(std::string_view logical, std::string_view resolved,
                                  std::string_view bundleRoot) noexcept;
const char* SampLeaf(Resource resource) noexcept;
void ApplySampAssetOverride(Resolution& asset) noexcept;
bool HasSampTrackHeader(std::FILE* stream) noexcept;

struct CloseFile {
    void operator()(std::FILE* file) const noexcept { std::fclose(file); }
};
using File = std::unique_ptr<std::FILE, CloseFile>;

enum class StreamOrigin { Unspecified, External, Bundle };

struct OpenResult {
    File file;
    std::error_code error;
    StreamOrigin origin = StreamOrigin::Unspecified;
};

// These callbacks model Morgan-owned streams, not GTA handles or return codes.
// Bind original platform arguments in caller context and keep contexts alive during a call.
// Implementations must be thread-safe when shared and return either a file or an error.
struct OriginalOpen {
    void* context;
    OpenResult (*call)(void*, std::string_view, Access) noexcept;
};

struct ReplacementOpen {
    void* context;
    OpenResult (*call)(void*, const Resolution&) noexcept;
};

struct DebugDiagnostics {
    void* context = nullptr;
    void (*emit)(void*, Diagnostic) noexcept = nullptr;
};

OpenResult OpenFile(std::string_view path, Access access, OriginalOpen original,
                    ReplacementOpen replacement = {}, Mode mode = Mode::OriginalOnly,
                    DebugDiagnostics diagnostics = {}) noexcept;

} // namespace morgan::fileio
