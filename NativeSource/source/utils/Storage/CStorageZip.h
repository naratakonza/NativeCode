#pragma once

#include "../FileIO/FileIO.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace morgan::fileio::storage {

struct GroupStats {
    bool present = false;
    bool valid = false;
    std::uint64_t files = 0;
    std::uint64_t bytes = 0;
};
using GroupInventory = std::array<GroupStats, GroupCount>;

struct AssetSpec {
    std::uint64_t bytes;
    std::string_view sha256;
};

using SampChecks = std::array<AssetSpec, 2>;
inline constexpr SampChecks SupportedAssets{{
    {96997, "b36890ecf1cb3255921b4ac15600ebb79cfbf2490aa23abccf17b1661c4b060b"},
    {184320, "adf3b20e847316aefde6cbf47eeae2e77c8e9185e096c8ed9668e60bd2ff11a4"},
}};
inline constexpr std::array<Resource, 2> AssetResources{Resource::MainScript, Resource::ScriptImage};

struct ZipEntry {
    std::uint64_t offset;
    std::uint64_t packed;
    std::uint64_t bytes;
    std::uint32_t crc;
    std::uint16_t method;
    std::string path;
    std::size_t group;
};

inline constexpr std::string_view SampSettingsArchivePath = "SAMP/settings.json";

using ZipAssets = std::vector<ZipEntry>;
using Progress = void (*)(void*, std::size_t);
using CancelCheck = bool (*)(void*);

const AssetSpec* TrustedSampAsset(std::string_view path, const SampChecks& checks) noexcept;
ZipAssets InspectZip(std::FILE* zip, const SampChecks& checks);
void ExtractAsset(std::FILE* zip, const ZipEntry& entry, const AssetSpec& asset, std::FILE* output,
                  Progress progress = nullptr, void* context = nullptr, CancelCheck cancelled = nullptr);
bool MatchesAsset(std::FILE* file, const AssetSpec& asset);

} // namespace morgan::fileio::storage
