#include "CStorageZip.h"
#include "CStorageSettings.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
#include <zlib.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#else
#include <CommonCrypto/CommonDigest.h>
#endif

namespace morgan::fileio::storage {
namespace {

constexpr std::uint64_t MaxArchiveBytes = 8ull << 30;
constexpr std::uint64_t MaxExpandedBytes = 12ull << 30;
constexpr std::uint64_t MaxFileBytes = 6ull << 30;
constexpr std::size_t MaxEntries = 8192;
constexpr std::uint32_t MaxRatio = 1024;
constexpr std::uint16_t DataDescriptor = 0x0008;
constexpr std::size_t BlockBytes = 32768;

void Require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

class Sha256 {
public:
    Sha256() {
#if defined(_WIN32)
        Require(BCryptOpenAlgorithmProvider(&algorithm_, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0,
                "SHA-256 initialization failed");
        if (BCryptCreateHash(algorithm_, &hash_, nullptr, 0, nullptr, 0, 0) != 0) {
            BCryptCloseAlgorithmProvider(algorithm_, 0);
            throw std::runtime_error("SHA-256 initialization failed");
        }
#else
        Require(CC_SHA256_Init(&hash_) == 1, "SHA-256 initialization failed");
#endif
    }

    ~Sha256() {
#if defined(_WIN32)
        BCryptDestroyHash(hash_);
        BCryptCloseAlgorithmProvider(algorithm_, 0);
#endif
    }

    Sha256(const Sha256&) = delete;
    Sha256& operator=(const Sha256&) = delete;

    void Update(unsigned char* data, unsigned count) {
#if defined(_WIN32)
        Require(BCryptHashData(hash_, data, count, 0) == 0, "SHA-256 update failed");
#else
        Require(CC_SHA256_Update(&hash_, data, count) == 1, "SHA-256 update failed");
#endif
    }

    bool Equals(std::string_view expected) {
        unsigned char bytes[32];
#if defined(_WIN32)
        Require(BCryptFinishHash(hash_, bytes, sizeof(bytes), 0) == 0, "SHA-256 read failed");
#else
        Require(CC_SHA256_Final(bytes, &hash_) == 1, "SHA-256 read failed");
#endif
        constexpr char hex[] = "0123456789abcdef";
        if (expected.size() != sizeof(bytes) * 2) return false;
        for (std::size_t i = 0; i < sizeof(bytes); ++i) {
            if (expected[2 * i] != hex[bytes[i] >> 4] || expected[2 * i + 1] != hex[bytes[i] & 15]) {
                return false;
            }
        }
        return true;
    }

private:
#if defined(_WIN32)
    BCRYPT_ALG_HANDLE algorithm_ = nullptr;
    BCRYPT_HASH_HANDLE hash_ = nullptr;
#else
    CC_SHA256_CTX hash_{};
#endif
};

std::uint16_t U16(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}
std::uint32_t U32(const unsigned char* p) {
    return U16(p) | (static_cast<std::uint32_t>(U16(p + 2)) << 16);
}
std::uint64_t U64(const unsigned char* p) {
    return U32(p) | (static_cast<std::uint64_t>(U32(p + 4)) << 32);
}

#if defined(_WIN32)
int Seek(std::FILE* file, std::int64_t offset, int origin) { return ::_fseeki64(file, offset, origin); }
std::int64_t Tell(std::FILE* file) { return ::_ftelli64(file); }
#else
int Seek(std::FILE* file, std::int64_t offset, int origin) { return ::fseeko(file, offset, origin); }
std::int64_t Tell(std::FILE* file) { return ::ftello(file); }
#endif

void Read(std::FILE* file, void* output, std::size_t count) {
    Require(count == 0 || std::fread(output, 1, count, file) == count, "Truncated ZIP or read error");
}
void ReadAt(std::FILE* file, std::uint64_t offset, void* output, std::size_t count) {
    Require(offset <= MaxArchiveBytes && Seek(file, static_cast<std::int64_t>(offset), SEEK_SET) == 0,
            "Cannot seek in ZIP");
    Read(file, output, count);
}

struct Directory { std::uint64_t offset, bytes, count; };

Directory LocateDirectory(std::FILE* zip) {
    Require(Seek(zip, 0, SEEK_END) == 0, "ZIP is not seekable");
    const auto length = Tell(zip);
    Require(length >= 22 && static_cast<std::uint64_t>(length) <= MaxArchiveBytes, "Invalid ZIP size");
    const auto tailBytes = static_cast<std::size_t>(std::min<std::int64_t>(length, 65557));
    const auto tailOffset = static_cast<std::uint64_t>(length) - tailBytes;
    std::vector<unsigned char> tail(tailBytes);
    ReadAt(zip, tailOffset, tail.data(), tail.size());
    std::size_t eocd = tail.size() - 22;
    while (U32(tail.data() + eocd) != 0x06054b50 ||
           eocd + 22 + U16(tail.data() + eocd + 20) != tail.size()) {
        Require(eocd > 0, "Missing ZIP end record");
        --eocd;
    }
    const auto* end = tail.data() + eocd;
    const auto endOffset = tailOffset + eocd;
    Require(U16(end + 4) == 0 && U16(end + 6) == 0 && U16(end + 8) == U16(end + 10),
            "Multi-disk ZIP is not supported");
    Directory dir{U32(end + 16), U32(end + 12), U16(end + 10)};
    auto directoryEnd = endOffset;
    if (dir.count == 0xffff || dir.offset == 0xffffffff || dir.bytes == 0xffffffff ||
        dir.offset > endOffset || dir.bytes != endOffset - dir.offset) {
        Require(endOffset >= 20, "Invalid ZIP64 locator");
        unsigned char locator[20];
        ReadAt(zip, endOffset - sizeof(locator), locator, sizeof(locator));
        const auto zip64 = U64(locator + 8);
        Require(U32(locator) == 0x07064b50 && U32(locator + 4) == 0 && U32(locator + 16) == 1 &&
                zip64 <= endOffset - sizeof(locator) && endOffset - sizeof(locator) - zip64 >= 56,
                "Invalid ZIP64 locator");
        unsigned char record[56];
        ReadAt(zip, zip64, record, sizeof(record));
        Require(U32(record) == 0x06064b50 && U64(record + 4) == endOffset - sizeof(locator) - zip64 - 12 &&
                U32(record + 16) == 0 && U32(record + 20) == 0 && U64(record + 24) == U64(record + 32),
                "Invalid ZIP64 end record");
        const Directory wide{U64(record + 48), U64(record + 40), U64(record + 32)};
        Require((dir.count == 0xffff || dir.count == wide.count) &&
                (dir.offset == 0xffffffff || dir.offset == wide.offset) &&
                (dir.bytes == 0xffffffff || dir.bytes == wide.bytes), "ZIP64 end records disagree");
        dir = wide;
        directoryEnd = zip64;
    }
    Require(dir.count > 0 && dir.count <= MaxEntries && dir.offset <= directoryEnd &&
            dir.bytes == directoryEnd - dir.offset, "Invalid ZIP directory bounds or entry count");
    return dir;
}

struct Zip64Values { std::uint64_t bytes, packed, offset; };

Zip64Values ParseExtras(const unsigned char* extra, std::size_t length, Zip64Values values) {
    const bool needSize = values.bytes == 0xffffffff;
    const bool needPacked = values.packed == 0xffffffff;
    const bool needOffset = values.offset == 0xffffffff;
    bool found = false;
    while (length) {
        Require(length >= 4, "Malformed ZIP extra field");
        const auto type = U16(extra);
        const std::size_t count = U16(extra + 2);
        Require(count <= length - 4 && type != 0x000d && type != 0x756e && type != 0x7075,
                "Unsupported ZIP link or alternate path metadata");
        if (type == 1) {
            const auto required = 8u * (needSize + needPacked + needOffset);
            Require(!found && count == required && count > 0, "Invalid ZIP64 extra field");
            found = true;
            const auto* words = extra + 4;
            if (needSize) { values.bytes = U64(words); words += 8; }
            if (needPacked) { values.packed = U64(words); words += 8; }
            if (needOffset) values.offset = U64(words);
        }
        extra += 4 + count;
        length -= 4 + count;
    }
    Require(found || !(needSize || needPacked || needOffset), "Missing ZIP64 sizes");
    return values;
}

std::uint64_t CheckDescriptor(std::FILE* zip, std::uint64_t end, std::uint64_t limit, const ZipEntry& entry, bool wide) {
    const std::uint64_t baseBytes = wide ? 20 : 12;
    const auto available = static_cast<std::size_t>(std::min(limit - end, baseBytes + 4));
    Require(available >= baseBytes, "Missing ZIP data descriptor");
    unsigned char descriptor[24];
    ReadAt(zip, end, descriptor, available);
    const auto matches = [&](const unsigned char* p) {
        return U32(p) == entry.crc &&
               (wide ? U64(p + 4) : U32(p + 4)) == entry.packed &&
               (wide ? U64(p + 12) : U32(p + 8)) == entry.bytes;
    };
    if (available >= baseBytes + 4 && U32(descriptor) == 0x08074b50 && matches(descriptor + 4)) {
        return end + baseBytes + 4;
    }
    Require(matches(descriptor), "ZIP data descriptor does not match");
    return end + baseBytes;
}

std::uint64_t CheckLocal(std::FILE* zip, std::uint64_t offset, std::uint64_t limit, std::string_view name,
                         std::uint16_t flags, ZipEntry& entry, bool wideSizes) {
    unsigned char local[30];
    Require(offset <= limit && limit - offset >= sizeof(local), "Local ZIP header out of bounds");
    ReadAt(zip, offset, local, sizeof(local));
    Require(U32(local) == 0x04034b50 && U16(local + 6) == flags && U16(local + 8) == entry.method &&
            U16(local + 26) == name.size(), "ZIP local and central headers disagree");
    const auto extraBytes = U16(local + 28);
    const auto data = offset + sizeof(local) + name.size() + extraBytes;
    Require(data <= limit && entry.packed <= limit - data, "ZIP payload out of bounds");
    std::vector<unsigned char> localName(name.size() + extraBytes);
    Read(zip, localName.data(), localName.size());
    Require(name == std::string_view(reinterpret_cast<const char*>(localName.data()), name.size()),
            "ZIP local filename differs");
    const auto rawSize = U32(local + 22);
    const auto rawPacked = U32(local + 18);
    const auto values = ParseExtras(localName.data() + name.size(), extraBytes, {rawSize, rawPacked, 0});
    entry.offset = data;
    const auto end = data + entry.packed;
    if (!(flags & DataDescriptor)) {
        Require(U32(local + 14) == entry.crc && values.packed == entry.packed && values.bytes == entry.bytes,
                "ZIP local sizes or CRC differ");
        return end;
    }
    Require((U32(local + 14) == 0 || U32(local + 14) == entry.crc) &&
            (values.packed == 0 || values.packed == entry.packed) &&
            (values.bytes == 0 || values.bytes == entry.bytes), "ZIP streamed header differs");
    return CheckDescriptor(zip, end, limit, entry, wideSizes || rawSize == 0xffffffff || rawPacked == 0xffffffff);
}

class Inflater {
public:
    Inflater() { Require(inflateInit2(&stream, -MAX_WBITS) == Z_OK, "Cannot initialize ZIP inflater"); }
    ~Inflater() { inflateEnd(&stream); }
    z_stream stream{};
};

void CheckDirectoryData(std::FILE* zip, const ZipEntry& entry) {
    Require(entry.bytes == 0 && entry.crc == 0, "Invalid ZIP directory entry");
    if (entry.method == 0) return;
    unsigned char packed[64], output;
    Require(entry.packed > 0 && entry.packed <= sizeof(packed), "Invalid compressed directory");
    ReadAt(zip, entry.offset, packed, static_cast<std::size_t>(entry.packed));
    Inflater inflater;
    inflater.stream.next_in = packed;
    inflater.stream.avail_in = static_cast<uInt>(entry.packed);
    inflater.stream.next_out = &output;
    inflater.stream.avail_out = 1;
    Require(inflate(&inflater.stream, Z_FINISH) == Z_STREAM_END && inflater.stream.total_out == 0 &&
            inflater.stream.avail_in == 0, "Invalid compressed directory");
}

std::string Fold(std::string_view path) {
    std::string result(path);
    for (auto& c : result) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    return result;
}

struct SeenPath { std::string exact; bool directory; bool explicitEntry; };

void RegisterPath(std::map<std::string, SeenPath>& seen, std::string_view path, bool directory) {
    for (auto slash = path.find('/'); slash != std::string_view::npos; slash = path.find('/', slash + 1)) {
        const auto parent = path.substr(0, slash);
        auto [at, fresh] = seen.emplace(Fold(parent), SeenPath{std::string(parent), true, false});
        Require(fresh || (at->second.directory && at->second.exact == parent), "ZIP parent path collision");
    }
    auto [at, fresh] = seen.emplace(Fold(path), SeenPath{std::string(path), directory, true});
    if (fresh) return;
    Require(directory && at->second.directory && at->second.exact == path && !at->second.explicitEntry,
            "Duplicate or case-colliding ZIP path");
    at->second.explicitEntry = true;
}

} // namespace

const AssetSpec* TrustedSampAsset(std::string_view path, const SampChecks& checks) noexcept {
    if (path == "SAMP/main.scm") return &checks[0];
    if (path == "SAMP/script.img") return &checks[1];
    return nullptr;
}

ZipAssets InspectZip(std::FILE* zip, const SampChecks& checks) {
    const auto directory = LocateDirectory(zip);
    const auto end = directory.offset + directory.bytes;
    auto position = directory.offset;
    std::uint64_t total = 0;
    ZipAssets assets;
    std::map<std::string, SeenPath> seen;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;
    bool samp = false;
    unsigned sampPair = 0;
    for (std::uint64_t i = 0; i < directory.count; ++i) {
        unsigned char header[46];
        Require(end - position >= sizeof(header), "Truncated ZIP directory");
        ReadAt(zip, position, header, sizeof(header));
        Require(U32(header) == 0x02014b50 && U16(header + 34) == 0, "Invalid ZIP directory entry");
        const auto nameBytes = U16(header + 28);
        const auto extraBytes = U16(header + 30);
        const auto extent = sizeof(header) + nameBytes + extraBytes + U16(header + 32);
        Require(extent <= end - position && nameBytes >= 4 && nameBytes <= 241, "Unsupported ZIP path");
        std::vector<unsigned char> meta(static_cast<std::size_t>(nameBytes) + extraBytes);
        Read(zip, meta.data(), meta.size());
        const std::string_view name(reinterpret_cast<const char*>(meta.data()), nameBytes);
        const bool isDir = name.back() == '/';
        const auto path = isDir ? name.substr(0, name.size() - 1) : name;
        const bool sampSettings = !isDir && path == SampSettingsArchivePath;
        Require(sampSettings || SafeAssetPath(path), "Unsafe ZIP path");
        const auto groupName = path.substr(0, path.find('/'));
        const auto found = std::find(GroupNames.begin(), GroupNames.end(), groupName);
        const bool rootConfig = !isDir && IsRootDataConfig(path);
        Require(rootConfig || found != GroupNames.end(), "Unsupported asset group");
        const auto group = rootConfig ? DataGroup : static_cast<std::size_t>(found - GroupNames.begin());
        Require(isDir || sampSettings || AllowedAssetPath(path, group), "File must be inside its asset group");
        RegisterPath(seen, path, isDir);

        const auto attr = U32(header + 38);
        const auto type = (attr >> 16) & 0170000;
        Require((type == 0 || type == (isDir ? 0040000u : 0100000u)) &&
                !(attr & 0x408) && (isDir || !(attr & 0x10)), "ZIP contains a symlink or special file");
        const auto flags = U16(header + 8);
        const auto size = U32(header + 24), packed = U32(header + 20);
        const auto sizes = ParseExtras(meta.data() + nameBytes, extraBytes, {size, packed, U32(header + 42)});
        ZipEntry entry{0, sizes.packed, sizes.bytes, U32(header + 16), U16(header + 10), std::string(path), group};
        Require((flags & ~0x080eu) == 0 && (entry.method == 0 || entry.method == 8) &&
                (entry.method == 8 || (entry.packed == entry.bytes && (flags & 6) == 0)),
                "Encrypted or unsupported ZIP compression");
        Require(entry.packed <= MaxArchiveBytes && entry.bytes <= MaxFileBytes &&
                entry.bytes <= MaxExpandedBytes - total && entry.bytes <= entry.packed * MaxRatio,
                "ZIP expansion limit exceeded");
        total += entry.bytes;
        const auto finish = CheckLocal(zip, sizes.offset, directory.offset, name, flags, entry,
                                       size == 0xffffffff || packed == 0xffffffff);
        ranges.emplace_back(sizes.offset, finish);
        if (isDir) {
            CheckDirectoryData(zip, entry);
        } else {
            Require(entry.bytes != 0, "Empty asset file");
            Require(!sampSettings || entry.bytes <= MaxSampSettingsBytes, "SAMP settings file is too large");
            if (group == 0) samp = true;
            if (const auto* trusted = TrustedSampAsset(path, checks)) {
                Require(entry.bytes == trusted->bytes, "SAMP file size differs from trusted asset");
                ++sampPair;
            }
            assets.push_back(std::move(entry));
        }
        position += extent;
    }
    Require(position == end && !assets.empty() && (!samp || sampPair == 2), "Incomplete asset ZIP or SAMP pair");
    std::sort(ranges.begin(), ranges.end());
    position = 0;
    for (const auto& range : ranges) {
        Require(range.first == position, "Overlapping or unlisted ZIP data");
        position = range.second;
    }
    Require(position == directory.offset, "Unexpected bytes before ZIP directory");
    return assets;
}

void ExtractAsset(std::FILE* zip, const ZipEntry& entry, const AssetSpec& asset, std::FILE* output,
                  Progress progress, void* context, CancelCheck cancelled) {
    Require(Seek(zip, static_cast<std::int64_t>(entry.offset), SEEK_SET) == 0, "Cannot seek to ZIP entry");
    Sha256 sha;
    auto crc = crc32(0, Z_NULL, 0);
    std::uint64_t produced = 0;
    const bool sampSettings = entry.path == SampSettingsArchivePath;
    std::string settings;
    if (sampSettings) {
        Require(entry.bytes <= MaxSampSettingsBytes, "SAMP settings file is too large");
        settings.reserve(static_cast<std::size_t>(entry.bytes));
    }
    auto write = [&](unsigned char* data, unsigned count) {
        Require(!cancelled || !cancelled(context), "Asset import cancelled");
        Require(count <= entry.bytes - produced, "ZIP produced more data than declared");
        Require(std::fwrite(data, 1, count, output) == count, "Cannot write staging file");
        crc = crc32(crc, data, count);
        if (!asset.sha256.empty()) sha.Update(data, count);
        if (sampSettings) settings.append(reinterpret_cast<const char*>(data), count);
        produced += count;
        if (progress && count) progress(context, count);
    };
    unsigned char input[BlockBytes];
    auto remaining = entry.packed;
    if (entry.method == 0) {
        while (remaining) {
            const auto count = static_cast<unsigned>(std::min<std::uint64_t>(remaining, sizeof(input)));
            Read(zip, input, count);
            write(input, count);
            remaining -= count;
        }
    } else {
        Inflater inflater;
        auto& stream = inflater.stream;
        unsigned char decoded[BlockBytes];
        for (;;) {
            if (stream.avail_in == 0 && remaining) {
                const auto count = static_cast<unsigned>(std::min<std::uint64_t>(remaining, sizeof(input)));
                Read(zip, input, count);
                stream.next_in = input;
                stream.avail_in = count;
                remaining -= count;
            }
            stream.next_out = decoded;
            stream.avail_out = sizeof(decoded);
            const int status = inflate(&stream, Z_NO_FLUSH);
            Require(status == Z_OK || status == Z_STREAM_END, "Invalid deflate stream");
            write(decoded, static_cast<unsigned>(sizeof(decoded) - stream.avail_out));
            if (status == Z_STREAM_END) {
                Require(stream.avail_in == 0 && remaining == 0, "Unexpected trailing deflate data");
                break;
            }
        }
    }
    Require(produced == asset.bytes && crc == entry.crc, "Asset size or ZIP CRC does not match");
    Require(asset.sha256.empty() || sha.Equals(asset.sha256), "Asset SHA-256 does not match trusted file");
    if (sampSettings) {
        rapidjson::Document document;
        Require(ParseSampSettings(settings, document), "Invalid SAMP settings.json");
    }
}

bool MatchesAsset(std::FILE* file, const AssetSpec& asset) {
    Sha256 sha;
    auto remaining = asset.bytes;
    unsigned char buffer[BlockBytes];
    while (remaining) {
        const auto count = static_cast<unsigned>(std::min<std::uint64_t>(remaining, sizeof(buffer)));
        if (std::fread(buffer, 1, count, file) != count) return false;
        if (!asset.sha256.empty()) sha.Update(buffer, count);
        remaining -= count;
    }
    return std::fgetc(file) == EOF && !std::ferror(file) &&
           (asset.sha256.empty() || sha.Equals(asset.sha256));
}

} // namespace morgan::fileio::storage
