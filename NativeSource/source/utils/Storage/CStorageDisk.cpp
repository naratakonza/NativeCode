#include "CStorageDisk.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

namespace morgan::fileio::storage {
namespace {

constexpr char LegacyRoot[] = "GameAssets";
constexpr int DirectoryFlags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;

void Require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

bool ExistsAt(int parent, const char* name) {
    struct stat info {};
    return ::fstatat(parent, name, &info, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT;
}

struct FileRecord {
    std::string path;
    std::uint64_t size;
};

bool ScanFiles(int root, std::size_t group, const std::string& prefix, unsigned depth,
               std::vector<FileRecord>& files, const SampChecks& checks) {
    if (depth > 12) return false;
    FileDescriptor scan(::openat(root, ".", DirectoryFlags));
    if (scan.get() < 0) return false;
    DIR* dir = ::fdopendir(scan.get());
    if (!dir) return false;
    scan.release();
    struct CloseDirectory {
        void operator()(DIR* value) const noexcept { ::closedir(value); }
    };
    const std::unique_ptr<DIR, CloseDirectory> owner(dir);
    for (;;) {
        errno = 0;
        const auto* entry = ::readdir(dir);
        if (!entry) return errno == 0;
        const std::string name(entry->d_name);
        if (name == "." || name == ".." ||
            (group == 0 && depth == 0 && name == "manifest.version")) continue;
        const auto path = prefix + "/" + name;
        if (IsSampStatePath(path)) continue;
        struct stat info {};
        if (::fstatat(root, name.c_str(), &info, AT_SYMLINK_NOFOLLOW) != 0) return false;
        if (S_ISDIR(info.st_mode)) {
            const FileDescriptor next(OpenExactDirectoryAt(root, name.c_str()));
            if (next.get() < 0 || !ScanFiles(next.get(), group, path, depth + 1, files, checks)) return false;
            continue;
        }
        if (!S_ISREG(info.st_mode) || info.st_nlink != 1 || (info.st_mode & 0777) != 0400 ||
            info.st_size <= 0 || static_cast<std::uint64_t>(info.st_size) > (6ull << 30) ||
            files.size() >= 8192 || !AllowedAssetPath(path, group)) return false;
        auto file = OpenRegularFileAt(root, name.c_str());
        if (!file.file) return false;
        const auto* trusted = TrustedSampAsset(path, checks);
        if (trusted ? !MatchesAsset(file.file.get(), *trusted) : std::fgetc(file.file.get()) == EOF) return false;
        files.push_back({path, static_cast<std::uint64_t>(info.st_size)});
    }
}

bool Contains(const std::vector<FileRecord>& files, std::string_view path) {
    return std::any_of(files.begin(), files.end(), [&](const FileRecord& f) { return f.path == path; });
}

constexpr std::array<std::array<const char*, 3>, GroupCount> required{{
        {{"SAMP/main.scm","SAMP/script.img",nullptr}},
        {{"anim/anim.img","anim/cuts.img","anim/ped.ifp"}},
        {{"audio/config/banklkup.dat","audio/sfx/genrl.osw","audio/sfx/genrl.osw.idx"}},
        {{"data/gta.dat","data/script/mainv1.scm","data/script/scriptv1.img"}},
        {{"models/coll/peds.col","models/coll/vehicles.col","models/effects.fxp"}},
        {{"texdb/gta3.img","texdb/gta3/gta3.txt",nullptr}},
        {{"text/american.gxt","text/american.alp",nullptr}},
        {{"textures/fonts/font1.bmp","textures/fonts/font2.bmp",nullptr}},
    }};

bool HasRequiredFiles(std::size_t group, const std::vector<FileRecord>& files) {
    for (const auto* path : required[group]) if (path && !Contains(files, path)) return false;
    return true;
}

bool ValidateRootDataConfigs(int root, GroupStats* status = nullptr) {
    bool valid = true;
    for (const auto* name : RootDataConfigs) {
        auto file = OpenRegularFileAt(root, name);
        struct stat info {};
        if (!file.file || ::fstat(::fileno(file.file.get()), &info) != 0 ||
            (info.st_mode & 0777) != 0400 || info.st_size <= 0 ||
            static_cast<std::uint64_t>(info.st_size) > (6ull << 30) || std::fgetc(file.file.get()) == EOF) {
            valid = false;
            continue;
        }
        if (status) {
            ++status->files;
            status->bytes += static_cast<std::uint64_t>(info.st_size);
        }
    }
    return valid;
}

void CheckCapacity(int root, std::uint64_t expanded) {
    struct statvfs disk {};
    Require(::fstatvfs(root, &disk) == 0 && disk.f_frsize != 0, "Cannot check storage capacity");
    const auto budget = expanded + (8ull << 20);
    Require(disk.f_bavail >= (budget + disk.f_frsize - 1) / disk.f_frsize, "Not enough storage to import assets");
}

FileDescriptor OpenParent(int root, std::string_view path, bool create) {
    FileDescriptor directory(::openat(root, ".", DirectoryFlags));
    while (directory.get() >= 0) {
        const auto slash = path.find('/');
        if (slash == std::string_view::npos) return directory;
        const std::string part(path.substr(0, slash));
        if (create && ::mkdirat(directory.get(), part.c_str(), 0700) != 0 && errno != EEXIST) break;
        directory.reset(OpenExactDirectoryAt(directory.get(), part.c_str()));
        path.remove_prefix(slash + 1);
    }
    return FileDescriptor(-1);
}

std::string Leaf(std::string_view path) {
    const auto slash = path.find_last_of('/');
    return std::string(path.substr(slash == std::string_view::npos ? 0 : slash + 1));
}

File NewFile(int root, std::string_view path) {
    const auto parent = OpenParent(root, path, true);
    Require(parent.get() >= 0, "Cannot create asset staging directory");
    const auto leaf = Leaf(path);
    FileDescriptor descriptor(::openat(parent.get(), leaf.c_str(), O_WRONLY | O_CREAT | O_EXCL |
                                       O_NOFOLLOW | O_CLOEXEC, 0600));
    Require(descriptor.get() >= 0, "Cannot create staging file");
    File file(::fdopen(descriptor.get(), "wb"));
    Require(static_cast<bool>(file), "Cannot open staging file");
    descriptor.release();
    return file;
}

void SealFile(File file, mode_t mode = 0400) {
    const int fd = ::fileno(file.get());
    Require(std::fflush(file.get()) == 0 && ::fchmod(fd, mode) == 0 && ::fsync(fd) == 0,
            "Cannot persist staging file");
    Require(std::fclose(file.release()) == 0, "Cannot close staging file");
}

void CopyExisting(int source, int stage, const FileRecord& file) {
    const auto suffix = std::string_view(file.path).substr(file.path.find('/') + 1);
    const auto parent = OpenParent(source, suffix, false);
    Require(parent.get() >= 0, "Cannot open existing asset directory");
    auto input = OpenRegularFileAt(parent.get(), Leaf(suffix).c_str());
    Require(static_cast<bool>(input.file), "Cannot open existing asset");
    auto output = NewFile(stage, file.path);
    unsigned char buffer[32768];
    for (auto left = file.size; left;) {
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(left, sizeof(buffer)));
        Require(std::fread(buffer, 1, count, input.file.get()) == count &&
                std::fwrite(buffer, 1, count, output.get()) == count, "Cannot copy existing asset");
        left -= count;
    }
    Require(std::fgetc(input.file.get()) == EOF && !std::ferror(input.file.get()), "Existing asset changed");
    SealFile(std::move(output));
}

void ClearOwnedStage(int root, unsigned depth = 0) noexcept {
    if (depth > 16) return;
    FileDescriptor scan(::openat(root, ".", DirectoryFlags));
    if (scan.get() < 0) return;
    DIR* dir = ::fdopendir(scan.get());
    if (!dir) return;
    scan.release();
    while (const auto* entry = ::readdir(dir)) {
        const auto* name = entry->d_name;
        if (std::string_view(name) == "." || std::string_view(name) == "..") continue;
        const FileDescriptor child(OpenExactDirectoryAt(root, name));
        if (child.get() >= 0) {
            ClearOwnedStage(child.get(), depth + 1);
            ::unlinkat(root, name, AT_REMOVEDIR);
        } else {
            ::unlinkat(root, name, 0);
        }
    }
    ::closedir(dir);
}

class StagingDirectory {
public:
    explicit StagingDirectory(int parent) : parent_(parent) {
        for (int attempt = 0; attempt != 8; ++attempt) {
            std::snprintf(name_, sizeof(name_), ".import-%08x%08x", ::arc4random(), ::arc4random());
            if (::mkdirat(parent_, name_, 0700) == 0) {
                directory_.reset(OpenExactDirectoryAt(parent_, name_));
                if (directory_.get() >= 0) return;
                ::unlinkat(parent_, name_, AT_REMOVEDIR);
                break;
            }
            if (errno != EEXIST) break;
        }
        throw std::runtime_error("Cannot create asset staging directory");
    }

    ~StagingDirectory() {
        if (preserve_) return;
        ClearOwnedStage(directory_.get());
        ::unlinkat(parent_, name_, AT_REMOVEDIR);
    }

    int get() const noexcept { return directory_.get(); }

    void Publish(const std::array<bool, GroupCount>& groups, bool rootConfigs = false, bool useImportedSettings = false) {
        if (groups[0])
        {
            PreserveSampState(SampLogName);
            if (!useImportedSettings) PreserveSampState(SampSettingsName);
        }

        std::array<const char*, GroupCount + RootDataConfigs.size()> names{};
        std::size_t count = 0;
        for (std::size_t i = 0; i < GroupCount; ++i) {
            if (groups[i]) names[count++] = GroupNames[i];
        }
        if (rootConfigs) {
            for (const auto* name : RootDataConfigs) names[count++] = name;
        }

        std::array<int, names.size()> applied{};
        for (std::size_t i = 0; i < count; ++i) {
            const auto* name = names[i];
            if (::renameatx_np(get(), name, parent_, name, RENAME_EXCL) == 0) {
                applied[i] = 1;
                continue;
            }
            if (errno == EEXIST && ::renameatx_np(get(), name, parent_, name, RENAME_SWAP) == 0) {
                applied[i] = 2;
                continue;
            }
            for (auto j = i; j > 0;) {
                --j;
                if (applied[j] == 0) continue;
                const auto flags = applied[j] == 2 ? RENAME_SWAP : RENAME_EXCL;
                const auto* prior = names[j];
                if (::renameatx_np(parent_, prior, get(), prior, flags) != 0) preserve_ = true;
            }
            throw std::runtime_error(preserve_ ? "Asset publish rollback requires recovery" : "Cannot publish validated assets");
        }
    }

private:
    void PreserveSampState(const char* name)
    {
        const FileDescriptor current(OpenExactDirectoryAt(parent_, SampDirectory));
        if (current.get() < 0) return;

        struct stat info {};
        if (::fstatat(current.get(), name, &info, AT_SYMLINK_NOFOLLOW) != 0 || !S_ISREG(info.st_mode) || info.st_nlink != 1 || info.st_uid != ::geteuid()) return;

        const auto input = OpenRegularFileAt(current.get(), name);
        Require(static_cast<bool>(input.file), "Cannot read SAMP state file");
        const FileDescriptor replacement(OpenExactDirectoryAt(get(), SampDirectory));
        Require(replacement.get() >= 0, "Cannot open staged SAMP directory");
        if (std::string_view(name) == SampSettingsName) {
            Require(::unlinkat(replacement.get(), name, 0) == 0 || errno == ENOENT, "Cannot replace staged SAMP settings");
        }
        FileDescriptor output(::openat(replacement.get(), name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
        Require(output.get() >= 0, "Cannot preserve SAMP state file");
        File log(::fdopen(output.get(), "wb"));
        Require(static_cast<bool>(log), "Cannot write staged SAMP state file");
        output.release();

        char buffer[4096];
        while (const auto count = std::fread(buffer, 1, sizeof(buffer), input.file.get())) {
            Require(std::fwrite(buffer, 1, count, log.get()) == count, "Cannot copy SAMP state file");
        }
        Require(!std::ferror(input.file.get()) && std::fflush(log.get()) == 0, "Cannot finish copying SAMP state file");
    }

    int parent_;
    FileDescriptor directory_{-1};
    char name_[32]{};
    bool preserve_ = false;
};

bool InspectGroup(int groupRoot, std::size_t group, const SampChecks& checks, std::vector<FileRecord>& files, bool* present = nullptr) {
    const bool scanned = groupRoot >= 0 && ScanFiles(groupRoot, group, GroupNames[group], 0, files, checks);
    if (present && group == 0 && scanned && files.empty()) *present = false;
    return scanned && HasRequiredFiles(group, files);
}

} // namespace

bool ValidateAssetRoot(int root, const SampChecks& checks) {
    for (std::size_t i = 0; i < AssetResources.size(); ++i) {
        auto file = OpenSampLeafAt(root, AssetResources[i]);
        struct stat info {};
        if (!file.file || ::fstat(::fileno(file.file.get()), &info) != 0 ||
            (info.st_mode & 0777) != 0400 || !MatchesAsset(file.file.get(), checks[i])) return false;
    }
    return true;
}

FileDescriptor ValidateGameAssets(int morganRoot, const SampChecks& checks) {
    FileDescriptor root(OpenExactDirectoryAt(morganRoot, SampDirectory));
    if (root.get() >= 0 && ValidateAssetRoot(root.get(), checks)) return root;
    return FileDescriptor(-1);
}

bool HasRequiredBundleFiles(int bundle, std::size_t group) {
    for (const auto* path : required[group]) {
        if (!path) continue;
        const auto parent = OpenParent(bundle, path, false);
        if (parent.get() < 0) return false;
        auto result = OpenRegularFileAt(parent.get(), Leaf(path).c_str());
        if (!result.file || std::fgetc(result.file.get()) == EOF) return false;
    }
    return true;
}

GroupInventory InspectInstalledGroups(int morganRoot, const SampChecks& checks) {
    GroupInventory groups{};
    for (std::size_t i = 0; i < GroupCount; ++i) {
        const FileDescriptor root(OpenExactDirectoryAt(morganRoot, GroupNames[i]));
        auto& status = groups[i];
        status.present = root.get() >= 0 || ExistsAt(morganRoot, GroupNames[i]);
        std::vector<FileRecord> files;
        status.valid = InspectGroup(root.get(), i, checks, files, &status.present);
        status.files = files.size();
        for (const auto& file : files) status.bytes += file.size;
        if (i == DataGroup) status.valid = ValidateRootDataConfigs(morganRoot, &status) && status.valid;
    }
    return groups;
}

RestoredAssets RestoreGameAssets(int morganRoot, const SampChecks& checks) {
    FileDescriptor current(OpenExactDirectoryAt(morganRoot, SampDirectory));
    const bool accessible = current.get() >= 0;
    if (accessible && ValidateAssetRoot(current.get(), checks)) return {std::move(current), "restored", true};
    const FileDescriptor legacy(OpenExactDirectoryAt(morganRoot, LegacyRoot));
    const FileDescriptor old(OpenExactDirectoryAt(legacy.get(), SampDirectory));
    std::vector<FileRecord> files;
    if (!InspectGroup(old.get(), 0, checks, files)) {
        bool present = accessible || ExistsAt(morganRoot, SampDirectory);
        if (accessible) {
            files.clear();
            InspectGroup(current.get(), 0, checks, files, &present);
        }
        present = present || ExistsAt(morganRoot, LegacyRoot);
        return {FileDescriptor(-1), present ? "invalid" : "not_found", accessible};
    }
    std::uint64_t expanded = 0;
    for (const auto& file : files) expanded += file.size;
    CheckCapacity(morganRoot, expanded);
    StagingDirectory stage(morganRoot);
    for (const auto& file : files) CopyExisting(old.get(), stage.get(), file);
    Require(ValidateGameAssets(stage.get(), checks).get() >= 0, "Staged legacy assets failed validation");
    std::array<bool, GroupCount> groups{};
    groups[0] = true;
    stage.Publish(groups);
    auto migrated = ValidateGameAssets(morganRoot, checks);
    Require(migrated.get() >= 0, "Cannot reopen migrated SAMP assets");
    return {std::move(migrated), "migrated", true};
}

FileDescriptor ImportGameAssets(int morganRoot, std::FILE* zip, const SampChecks& checks,
                                Progress progress, void* context, InstallTrace* trace, CancelCheck cancelled,
                                std::uint64_t* totalBytes) {
    if (trace) { *trace = {}; trace->validation = "failed"; }
    const auto entries = InspectZip(zip, checks);
    std::uint64_t expanded = 0;
    std::array<bool, GroupCount> groups{};
    bool rootConfigs = false;
    for (const auto& entry : entries) {
        expanded += entry.bytes;
        if (IsRootDataConfig(entry.path)) rootConfigs = true;
        else groups[entry.group] = true;
    }

    if (totalBytes) *totalBytes = expanded;

    // Preserve optional SAMP resources when an existing installation receives the legacy two-file ZIP.
    const FileDescriptor existingSamp(OpenExactDirectoryAt(morganRoot, SampDirectory));
    std::vector<FileRecord> keep;
    const bool installedSamp = groups[0] && InspectGroup(existingSamp.get(), 0, checks, keep);
    if (installedSamp) {
        keep.erase(std::remove_if(keep.begin(), keep.end(), [&](const FileRecord& file) {
            return std::any_of(entries.begin(), entries.end(), [&](const ZipEntry& e) { return e.path == file.path; });
        }), keep.end());
        for (const auto& file : keep) expanded += file.size;
    } else {
        keep.clear();
    }
    CheckCapacity(morganRoot, expanded);
    StagingDirectory stage(morganRoot);
    for (const auto& entry : entries) {
        Require(!cancelled || !cancelled(context), "Asset import cancelled");
        auto file = NewFile(stage.get(), entry.path);
        const auto* trusted = TrustedSampAsset(entry.path, checks);
        ExtractAsset(zip, entry, trusted ? *trusted : AssetSpec{entry.bytes, {}},
                     file.get(), progress, context, cancelled);
        SealFile(std::move(file), entry.path == SampSettingsArchivePath ? 0600 : 0400);
    }
    for (const auto& file : keep) {
        Require(!cancelled || !cancelled(context), "Asset import cancelled");
        CopyExisting(existingSamp.get(), stage.get(), file);
    }
    if (trace) trace->extracted = true;
    for (std::size_t i = 0; i < GroupCount; ++i) {
        if (!groups[i]) continue;
        const FileDescriptor root(OpenExactDirectoryAt(stage.get(), GroupNames[i]));
        std::vector<FileRecord> files;
        if (!InspectGroup(root.get(), i, checks, files)) throw std::runtime_error(std::string("Missing or invalid required files in ") + GroupNames[i]);
    }
    if (rootConfigs || groups[DataGroup]) {
        Require(ValidateRootDataConfigs(rootConfigs ? stage.get() : morganRoot), "Missing or invalid root streaming configs");
    }
    if (trace) trace->validation = "passed";
    Require(!cancelled || !cancelled(context), "Asset import cancelled");
    if (trace) trace->publish = "failed";
    const bool useImportedSettings = !installedSamp && std::any_of(entries.begin(), entries.end(), [](const ZipEntry& entry) {
        return entry.path == SampSettingsArchivePath;
    });
    stage.Publish(groups, rootConfigs, useImportedSettings);
    if (trace) trace->publish = "published";
    return ValidateGameAssets(morganRoot, checks);
}

} // namespace morgan::fileio::storage
