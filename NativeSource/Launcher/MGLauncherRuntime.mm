#import "MGLauncherViewController.h"
#import "MGStartupTickGate.h"
#import "MGStartupKeychain.h"
#import "MGFlutter.h"

#import <dispatch/dispatch.h>

#include <charconv>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#include "../Main.h"
#include "../source/game_morgan/CLoadingScreen.h"
#include "../source/utils/FileIO/FileOpenDiagnostics.h"
#include "../source/utils/Memory/CHook.h"
#include "../source/utils/Storage/CStorage.h"
#include "../source/utils/Storage/PosixFileIO.h"

static NSString* MGAssetSourceName(morgan::fileio::AssetSource source) {
    using morgan::fileio::AssetSource;
    switch (source) {
        case AssetSource::Original: return @"Original";
        case AssetSource::Bundle: return @"Bundle";
        case AssetSource::External: return @"External";
    }
    return @"Unknown";
}

static NSString* MGImportStatusName(morgan::fileio::ImportStatus status) {
    using morgan::fileio::ImportStatus;
    switch (status) {
        case ImportStatus::NotImported: return @"Not Imported";
        case ImportStatus::Validating: return @"Validating";
        case ImportStatus::Ready: return @"Imported";
        case ImportStatus::Failed: return @"Failed";
    }
    return @"Unknown";
}

static NSString *MGSelectionName(StreamSelection selection) {
    switch (selection) {
        case StreamSelection::Waiting: return @"waiting (OriginalOnly)";
        case StreamSelection::WaitingAfterAssetRead: return @"waiting (early asset read observed)";
        case StreamSelection::EarlyRead: return @"blocked: matching read before mode selection";
        case StreamSelection::OriginalOnly: return @"OriginalOnly";
        case StreamSelection::OriginalOnlyAfterEarlyRead: return @"OriginalOnly (early matching read observed)";
        case StreamSelection::SampReadOnly: return @"SampReadOnly";
        case StreamSelection::ExternalOnly: return @"ExternalOnly";
    }
    return @"unknown";
}

static NSString *MGErrorName(StreamError error) {
    switch (error) {
        case StreamError::None: return @"none";
        case StreamError::InvalidRequest: return @"invalid_request";
        case StreamError::RecursiveOpen: return @"recursive_open";
        case StreamError::MissingReplacement: return @"missing_replacement";
        case StreamError::ReplacementRejected: return @"replacement_rejected";
        case StreamError::OriginalOpenFailed: return @"original_fopen_failed";
        case StreamError::AdapterError: return @"adapter_error";
    }
    return @"unknown";
}

static NSString *MGDiagnosticsText(NSString *sessionID, NSString *modeNote) {
    const auto snapshot = CHook::GameStreamSnapshot();
    const auto assets = morgan::fileio::CStorage::GetAssetStatus();
    NSMutableString* diagnostics = [NSMutableString stringWithFormat:
        @"Morgan FileOpen Diagnostics v3\nsession_id=%@\nmode_selection=%@\nlauncher_result=%@\n"
         "bridge_entry_calls=%llu\noriginal_fopen_calls=%llu\nmain_script_redirects=%llu\n"
         "script_image_redirects=%llu\nredirect_failures=%llu\nlast_error_category=%@\n"
         "asset_source=%@\nexternal_assets_ready=%d\nimport_status=%@\nimported_asset_version=%s\n"
         "external_main_script_opens=%llu\nexternal_script_image_opens=%llu\nbundle_fallback_opens=%llu\n"
         "zip_extract_completed=%d\nasset_validation_result=%s\nasset_publish_result=%s\n"
         "asset_state_restored=%d\nasset_restore_result=%s\nexternal_root_accessible=%d\n"
         "external_open_attempts=%llu\nexternal_open_successes=%llu\nexternal_open_failures=%llu\n"
         "external_open_error_category=%s\n",
        sessionID, MGSelectionName(snapshot.selection), modeNote,
        static_cast<unsigned long long>(snapshot.bridgeEntryCalls),
        static_cast<unsigned long long>(snapshot.originalFopenCalls),
        static_cast<unsigned long long>(snapshot.mainScriptRedirects),
        static_cast<unsigned long long>(snapshot.scriptImageRedirects),
        static_cast<unsigned long long>(snapshot.redirectFailures), MGErrorName(snapshot.lastError),
        MGAssetSourceName(assets.source), static_cast<int>(assets.externalReady),
        MGImportStatusName(assets.importStatus), "not_used",
        static_cast<unsigned long long>(assets.externalOpens.mainOpens),
        static_cast<unsigned long long>(assets.externalOpens.imageOpens),
        static_cast<unsigned long long>(assets.bundleFallbackOpens),
        static_cast<int>(assets.zipExtracted), assets.validationResult, assets.publishResult,
        static_cast<int>(std::string_view(assets.restoreResult) == "restored" ||
                         std::string_view(assets.restoreResult) == "migrated"),
        assets.restoreResult, static_cast<int>(assets.externalRootAccessible),
        static_cast<unsigned long long>(assets.externalOpens.Attempts()),
        static_cast<unsigned long long>(assets.externalOpens.Successes()),
        static_cast<unsigned long long>(assets.externalOpens.failures), assets.externalOpens.lastError];
    unsigned missing = 0;
    for (std::size_t i = 0; i < assets.groups.size(); ++i) {
        const auto& group = assets.groups[i];
        const bool samp = i == 0;
        const auto& reads = snapshot.groups[i];
        const char* status = group.present && !group.valid ? "Failed" :
            group.valid ? (reads.externalSuccesses ? "External opens observed"
                                                  : samp ? "Ready" : "Imported; runtime opens unverified")
                        : "Not Installed";
        if (!group.valid) ++missing;
        NSString* source = reads.externalSuccesses ? @"External observed" :
            samp ? MGAssetSourceName(assets.source) : assets.bundleGroups[i] ? @"Bundle present" : @"Unverified";
        const char* route = reads.externalSuccesses ? "MorganFileOpenBridge_external"
            : reads.attempts || reads.earlyReads ? "observed_via_MorganFileOpenBridge" : "not_observed";
        [diagnostics appendFormat:@"[asset_group:%s]\nasset_group_status=%s\nasset_group_source=%@\n"
            "asset_group_file_count=%llu\nasset_group_total_bytes=%llu\nasset_group_open_attempts=%llu\n"
            "asset_group_open_successes=%llu\nasset_group_open_failures=%llu\nasset_group_fallbacks=%llu\n"
            "asset_group_early_reads=%llu\nasset_group_external_successes=%llu\n"
            "asset_group_bundle_successes=%llu\nasset_loader_route=%s\n",
            morgan::fileio::storage::GroupNames[i], status, source,
            static_cast<unsigned long long>(group.files), static_cast<unsigned long long>(group.bytes),
            static_cast<unsigned long long>(reads.attempts),
            static_cast<unsigned long long>(reads.successes),
            static_cast<unsigned long long>(reads.failures),
            static_cast<unsigned long long>(reads.fallbacks),
            static_cast<unsigned long long>(reads.earlyReads),
            static_cast<unsigned long long>(reads.externalSuccesses),
            static_cast<unsigned long long>(reads.bundleSuccesses),
            route];
    }
    [diagnostics appendFormat:@"asset_import_result=%@\nexternal_asset_missing_count=%u\n",
                              MGImportStatusName(assets.importStatus), missing];
    return diagnostics;
}

static const char* MGStartupPhaseName(StartupPhase phase) {
    switch (phase) {
        case StartupPhase::None: return "none";
        case StartupPhase::FrameworkRegistered: return "framework_registered";
        case StartupPhase::UIKitLaunchFinished: return "uikit_did_finish_launch";
        case StartupPhase::TickGateInstalled: return "tick_gate_installed";
        case StartupPhase::TickGateRejected: return "tick_gate_rejected";
        case StartupPhase::FirstTickDeferred: return "first_gta_tick_deferred";
        case StartupPhase::GTAWindowObserved: return "gta_window_observed";
        case StartupPhase::LauncherPresented: return "launcher_presented";
        case StartupPhase::StorageValidationStarted: return "storage_validation_started";
        case StartupPhase::StorageValidationFinished: return "storage_validation_finished";
        case StartupPhase::ImportStarted: return "import_started";
        case StartupPhase::ImportFailed: return "import_failed";
        case StartupPhase::ImportCompleted: return "import_completed";
        case StartupPhase::HandoffCompleted: return "handoff_completed";
        case StartupPhase::ExternalModeSelected: return "external_only_selected";
        case StartupPhase::TickGateReleased: return "tick_gate_released";
        case StartupPhase::FirstTickStarted: return "gta_first_tick_started";
        case StartupPhase::FirstTickCompleted: return "gta_first_tick_completed";
    }
    return "unknown";
}

static const char* MGStartupReadResultName(StartupReadResult result) {
    switch (result) {
        case StartupReadResult::None: return "none";
        case StartupReadResult::InFlight: return "in_flight";
        case StartupReadResult::Opened: return "opened";
        case StartupReadResult::Missing: return "missing";
        case StartupReadResult::Failed: return "failed";
    }
    return "unknown";
}

static const char* MGRuntimeOpenRouteName(RuntimeOpenRoute route) {

    switch (route) {
        case RuntimeOpenRoute::OriginalOutsidePolicy: return "original_outside_policy";
        case RuntimeOpenRoute::OriginalAccess: return "original_access";
        case RuntimeOpenRoute::OriginalEndpointInactive: return "original_endpoint_inactive";
        case RuntimeOpenRoute::PathRejected: return "path_rejected";
        case RuntimeOpenRoute::AccessRejected: return "access_rejected";
        case RuntimeOpenRoute::ExternalOpened: return "external_opened";
        case RuntimeOpenRoute::ExternalFailed: return "external_failed";
        case RuntimeOpenRoute::BridgeFailed: return "bridge_failed";
    }
    return "unknown";
}

static const char* MGRuntimePathKindName(RuntimePathKind kind) {

    switch (kind) {
        case RuntimePathKind::Empty: return "empty";
        case RuntimePathKind::Rootless: return "rootless";
        case RuntimePathKind::Relative: return "relative_other";
        case RuntimePathKind::Asset: return "asset_root";
        case RuntimePathKind::BundleAbsolute: return "bundle_absolute";
        case RuntimePathKind::Absolute: return "absolute_redacted";
        case RuntimePathKind::DriveQualified: return "drive_prefix_redacted";
    }
    return "unknown";
}

static const char* MGResolvedPathKindName(morgan::fileio::ResolvedPathKind kind) {
    using morgan::fileio::ResolvedPathKind;
    switch (kind) {
        case ResolvedPathKind::Unchecked: return "unchecked";
        case ResolvedPathKind::Unavailable: return "unavailable";
        case ResolvedPathKind::BundleRootUnavailable: return "bundle_root_unavailable";
        case ResolvedPathKind::OutsideBundle: return "outside_bundle";
        case ResolvedPathKind::BundleNonAsset: return "bundle_non_asset";
        case ResolvedPathKind::BundleAsset: return "bundle_asset";
        case ResolvedPathKind::BundleInvalid: return "bundle_invalid";
        case ResolvedPathKind::BundleEcho: return "bundle_echo";
    }
    return "unknown";
}

static const char* MGResolverDecisionName(morgan::fileio::Disposition decision) {
    using morgan::fileio::Disposition;
    switch (decision) {
        case Disposition::Original: return "original";
        case Disposition::Redirect: return "external";
        case Disposition::Reject: return "reject";
    }
    return "unknown";
}

static const char* MGResolverIssueName(morgan::fileio::AssetPathIssue issue) {
    using morgan::fileio::AssetPathIssue;
    switch (issue) {
        case AssetPathIssue::None: return "none";
        case AssetPathIssue::InvalidPath: return "invalid_path";
        case AssetPathIssue::ConflictingPaths: return "conflicting_paths";
    }
    return "unknown";
}

static const char* MGPathRejectionName(morgan::fileio::PathRejection reason) {
    using morgan::fileio::PathRejection;
    switch (reason) {
        case PathRejection::None: return "none";
        case PathRejection::Empty: return "empty";
        case PathRejection::Length: return "length";
        case PathRejection::Depth: return "depth";
        case PathRejection::EmptyComponent: return "empty_component";
        case PathRejection::Traversal: return "traversal";
        case PathRejection::UnsafeCharacter: return "unsafe_character";
        case PathRejection::ReservedComponent: return "reserved_component";
        case PathRejection::TrailingSpace: return "trailing_space";
        case PathRejection::InvalidNormalization: return "invalid_normalization";
        case PathRejection::ConflictingPaths: return "conflicting_paths";
    }
    return "unknown";
}

static const char* MGNormalizationName(const RuntimeOpenEvent& event) {
    if (event.stockTrailingSpace) return "stock_trailing_space";
    if (event.decision == morgan::fileio::Disposition::Reject) return "rejected";
    return event.decision == morgan::fileio::Disposition::Redirect ? "canonical_asset" : "original";
}

static const char* MGRuntimeOpenModeName(RuntimeOpenMode mode) {

    switch (mode) {
        case RuntimeOpenMode::Read: return "read";
        case RuntimeOpenMode::Write: return "write";
        case RuntimeOpenMode::Update: return "update";
        case RuntimeOpenMode::Other: return "other";
    }
    return "unknown";
}

static NSString* MGStartupText(const StartupSnapshot& snapshot) {
    const char* gtaPhase = snapshot.Saw(StartupPhase::FirstTickCompleted) ? "gta_first_tick_completed"
        : snapshot.Saw(StartupPhase::FirstTickStarted) ? "gta_first_tick_started"
        : snapshot.Saw(StartupPhase::HandoffCompleted) ? "launcher_handoff_completed"
        : snapshot.Saw(StartupPhase::FirstTickDeferred) ? "first_display_tick_deferred"
        : snapshot.Saw(StartupPhase::GTAWindowObserved) ? "gta_window_observed"
        : snapshot.Saw(StartupPhase::UIKitLaunchFinished) ? "uikit_launch_finished" : "unobserved";
    NSMutableString *text = [NSMutableString stringWithFormat:
        @"[startup]\nstartup_phase=%s\nlauncher_presented=%d\ngta_initialization_phase=%s\n"
         "startup_tick_gate=%s\nstartup_phase_mask=0x%x\nearly_asset_open_attempts=%llu\n"
         "startup_dropped_events=%llu\nstartup_loader_route=%s\nlast_file_open_result=%s\n"
         "last_native_error=%d\nlast_asset_path=%s\n",
        MGStartupPhaseName(snapshot.lastPhase),
        static_cast<int>(snapshot.Saw(StartupPhase::LauncherPresented)), gtaPhase,
        snapshot.Saw(StartupPhase::TickGateInstalled) ? "installed" :
            snapshot.Saw(StartupPhase::TickGateRejected) ? "rejected" : "not_installed",
        static_cast<unsigned>(snapshot.phases), static_cast<unsigned long long>(snapshot.readAttempts),
        static_cast<unsigned long long>(snapshot.droppedEvents),
        snapshot.readAttempts ? "native_bridge_original_fopen" : "not_observed",
        MGStartupReadResultName(snapshot.readResult), snapshot.nativeError, snapshot.lastRead.path.data()];
    const auto group = snapshot.lastSuccessfulExternalGroup;
    const auto& groupNames = morgan::fileio::storage::GroupNames;
    [text appendFormat:@"external_only_mode_selected=%d\nstartup_tick_gate_released=%d\n"
         "gta_first_tick_completed=%d\nruntime_asset_open_attempts=%llu\n"
         "runtime_loader_route=%s\nlast_file_open_route=%s\nlast_successful_external_group=%s\n"
         "last_failed_asset_path=%s\nlast_failed_asset_errno=%d\n",
        static_cast<int>(snapshot.Saw(StartupPhase::ExternalModeSelected)),
        static_cast<int>(snapshot.Saw(StartupPhase::TickGateReleased)),
        static_cast<int>(snapshot.Saw(StartupPhase::FirstTickCompleted)),
        static_cast<unsigned long long>(snapshot.runtimeReadAttempts),
        snapshot.runtimeReadAttempts ? "native_bridge_external_only" : "not_observed",
        !snapshot.lastRead.sequence ? "not_observed" : snapshot.lastRead.external ? "external_only" : "original",
        group < groupNames.size() ? groupNames[group] : "none",
        snapshot.lastFailure.path.data(), snapshot.lastFailureErrno];
    for (const auto& path : snapshot.missingPaths) {
        if (path[0]) [text appendFormat:@"missing_asset_path=%s\n", path.data()];
    }
    const auto count = snapshot.runtimeOpenEventCount;
    const auto capacity = snapshot.runtimeOpenEvents.size();
    const auto first = count > capacity ? count - capacity : 0;
    [text appendFormat:@"runtime_open_events_recorded=%llu\n", static_cast<unsigned long long>(count)];
    for (auto i = first; i < count; ++i) {
        const auto& event = snapshot.runtimeOpenEvents[i % capacity];
        const auto& groups = morgan::fileio::storage::GroupNames;
        [text appendFormat:@"runtime_open_event=%llu route=%s path_kind=%s resolved_kind=%s path_source=%s group=%s "
                           "resolver_decision=%s resolver_issue=%s rejection_subreason=%s normalization_result=%s "
                           "mode=%s access=%u opened=%d errno=%d path=%s trusted_resolved_suffix=%s\n",
            static_cast<unsigned long long>(i + 1), MGRuntimeOpenRouteName(event.route),
            MGRuntimePathKindName(event.pathKind), MGResolvedPathKindName(event.resolvedKind),
            event.usedResolvedPath ? "bundle_resolved" : "logical",
            event.group < groups.size() ? groups[event.group] : "none",
            MGResolverDecisionName(event.decision), MGResolverIssueName(event.issue),
            MGPathRejectionName(event.rejection), MGNormalizationName(event),
            MGRuntimeOpenModeName(event.mode), event.access, static_cast<int>(event.opened),
            event.nativeError, event.path.data(), event.trustedResolvedSuffix.data()];
    }
    return text;
}

// Armed on the checkpoint queue before the asset-free test releases the GTA tick.
static constexpr std::size_t MGFailureSlots = 16;
static constexpr std::size_t MGFailureSlotBytes = 1024;
static constexpr char MGFailureHeader[] = "Morgan FileOpen Failure Journal v2\n";
using MGFailureFile = std::array<char, (MGFailureSlots + 1) * MGFailureSlotBytes>;
static int MGFailureJournalFD = -1; // Retained for the process after the sink is published.
static std::atomic_flag MGFailureWriting = ATOMIC_FLAG_INIT;
static std::uint64_t MGFailureSequence = 0;

static void MGSaveFileOpenFailure(const RuntimeOpenEvent& event) noexcept {
    // Never wait behind another failure. The normal open path does not call this sink.
    if (MGFailureWriting.test_and_set(std::memory_order_acquire)) return;
    const auto sequence = ++MGFailureSequence;
    std::array<char, MGFailureSlotBytes> slot{};
    timespec now{};
    (void)::clock_gettime(CLOCK_REALTIME, &now);
    const auto& groups = morgan::fileio::storage::GroupNames;
    const int length = std::snprintf(slot.data(), slot.size(),
        "failure_sequence=%llu unix_time=%lld.%03ld route=%s path_kind=%s resolved_kind=%s "
        "path_source=%s group=%s resolver_decision=%s resolver_issue=%s rejection_subreason=%s normalization_result=%s "
        "mode=%s access=%u opened=0 errno=%d path=%s trusted_resolved_suffix=%s\nend_failure=%llu\n",
        static_cast<unsigned long long>(sequence), static_cast<long long>(now.tv_sec),
        static_cast<long>(now.tv_nsec / 1000000), MGRuntimeOpenRouteName(event.route),
        MGRuntimePathKindName(event.pathKind), MGResolvedPathKindName(event.resolvedKind),
        event.usedResolvedPath ? "bundle_resolved" : "logical",
        event.group < groups.size() ? groups[event.group] : "none",
        MGResolverDecisionName(event.decision), MGResolverIssueName(event.issue),
        MGPathRejectionName(event.rejection), MGNormalizationName(event),
        MGRuntimeOpenModeName(event.mode), event.access, event.nativeError, event.path.data(),
        event.trustedResolvedSuffix.data(), static_cast<unsigned long long>(sequence));
    if (length > 0 && static_cast<std::size_t>(length) < slot.size()) {
        const auto offset = static_cast<off_t>((1 + (sequence - 1) % MGFailureSlots) * slot.size());
        (void)::pwrite(MGFailureJournalFD, slot.data(), slot.size(), offset);
    }
    MGFailureWriting.clear(std::memory_order_release);
}

static bool MGIsPrivateJournal(int fd) noexcept {
    struct stat status{};
    return fd >= 0 && ::fstat(fd, &status) == 0 && S_ISREG(status.st_mode) && status.st_nlink == 1 &&
        status.st_uid == ::geteuid() && (status.st_mode & 0777) == 0600;
}

static NSString *MGReadFailureJournal(NSURL *url) {
    const char* path = url.path.fileSystemRepresentation;
    if (!path) return nil;
    const morgan::fileio::FileDescriptor file(::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (!MGIsPrivateJournal(file.get())) return nil;
    MGFailureFile buffer{};
    if (::pread(file.get(), buffer.data(), buffer.size(), 0) != static_cast<ssize_t>(buffer.size())) return nil;
    const auto slotText = [&](std::size_t index) {
        const char* start = buffer.data() + index * MGFailureSlotBytes;
        const auto length = ::strnlen(start, MGFailureSlotBytes);
        return length < MGFailureSlotBytes ? std::string_view(start, length) : std::string_view{};
    };
    const auto header = slotText(0);
    if (header.substr(0, sizeof(MGFailureHeader) - 1) != MGFailureHeader) return nil;

    std::array<std::pair<std::uint64_t, std::string_view>, MGFailureSlots> records{};
    std::size_t count = 0;
    for (std::size_t i = 0; i < MGFailureSlots; ++i) {
        auto line = slotText(i + 1);
        constexpr std::string_view prefix = "failure_sequence=";
        if (line.substr(0, prefix.size()) != prefix) continue;
        std::uint64_t sequence = 0;
        const auto end = line.data() + line.size();
        const auto parsed = std::from_chars(line.data() + prefix.size(), end, sequence);
        if (parsed.ec != std::errc{} || parsed.ptr == end || *parsed.ptr != ' ' ||
            !sequence || (sequence - 1) % MGFailureSlots != i) continue;

        char trailer[48];
        const int length = std::snprintf(trailer, sizeof(trailer), "\nend_failure=%llu\n",
                                         static_cast<unsigned long long>(sequence));
        const auto size = static_cast<std::size_t>(length);
        if (length <= 0 || size >= sizeof(trailer) || size >= line.size() ||
            line.substr(line.size() - size) != std::string_view(trailer, size)) continue;
        line.remove_suffix(size - 1); // Keep the newline and omit the commit marker.
        records[count++] = {sequence, line};
    }
    if (!count) return nil;
    std::sort(records.begin(), records.begin() + count, [](const auto& a, const auto& b) { return a.first < b.first; });
    NSString *headerText = [[NSString alloc] initWithBytes:header.data()
        length:header.size() encoding:NSUTF8StringEncoding];
    NSMutableString *result = [headerText mutableCopy];
    for (std::size_t i = 0; i < count; ++i) {
        const auto line = records[i].second;
        NSString *entry = [[NSString alloc] initWithBytes:line.data() length:line.size() encoding:NSUTF8StringEncoding];
        if (entry) [result appendString:entry];
    }
    return result;
}

static bool MGArmFailureJournal(NSURL *url, NSString *sessionID) {
    if (MGFailureJournalFD >= 0) return true;
    const char* path = url.path.fileSystemRepresentation;
    const char* session = sessionID.UTF8String;
    if (!path || !session) return false;
    morgan::fileio::FileDescriptor file(::open(path, O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600));
    if (!MGIsPrivateJournal(file.get())) return false;
    MGFailureFile buffer{};
    const int length = std::snprintf(buffer.data(), MGFailureSlotBytes, "%ssession_id=%s\n",
                                     MGFailureHeader, session);
    if (length <= 0 || static_cast<std::size_t>(length) >= MGFailureSlotBytes ||
        ::pwrite(file.get(), buffer.data(), buffer.size(), 0) != static_cast<ssize_t>(buffer.size()) ||
        ::ftruncate(file.get(), static_cast<off_t>(buffer.size())) != 0) return false;
    MGFailureJournalFD = file.release();
    GameStartupTrace().SetFailureSink(&MGSaveFileOpenFailure);
    return true;
}

@interface MGStartupCheckpoint : NSObject
@property (nonatomic, copy) NSString *sessionID;
@property (atomic, copy) NSString *previous;
@property (atomic, copy) NSString *previousFailures;
@property (atomic, copy) NSString *failureJournalStatus;
@property (nonatomic, strong) NSURL *failureJournalURL;
@property (atomic, copy) NSString *lastSavedPhase;
@property (nonatomic, strong) dispatch_queue_t queue;
@property (nonatomic, strong) dispatch_source_t timer;
@property (nonatomic, copy) NSString *handoffNote;
@property (nonatomic, strong) NSURL *checkpointURL;
@property (nonatomic, copy) NSString *lastWritten;
@property (nonatomic) BOOL samplingGameplay;
- (void)start;
- (void)recordHandoff:(NSString *)note beforeTick:(dispatch_block_t)beforeTick;
@end

@implementation MGStartupCheckpoint

static constexpr NSUInteger MGCheckpointMaxBytes = 32 * 1024;

- (void)saveSnapshot {
    if (!self.checkpointURL) return;
    const auto startup = GameStartupTrace().Snapshot();
    NSString *runtime = self.handoffNote ? MGDiagnosticsText(self.sessionID, self.handoffNote) : @"";
    NSString *sample = [NSString stringWithFormat:
        @"Morgan Startup Checkpoint\nsession_id=%@\nlast_completed_startup_checkpoint=%s\n%@%@",
        self.sessionID, MGStartupPhaseName(startup.lastPhase), MGStartupText(startup), runtime];
    if ([sample isEqualToString:self.lastWritten]) return;
    NSString *stamped = [NSString stringWithFormat:@"checkpoint_unix_time=%.3f\n%@",
                         NSDate.date.timeIntervalSince1970, sample];
    if ([stamped lengthOfBytesUsingEncoding:NSUTF8StringEncoding] <= MGCheckpointMaxBytes &&
        [stamped writeToURL:self.checkpointURL atomically:YES encoding:NSUTF8StringEncoding error:nullptr]) {
        self.lastWritten = sample;
        self.lastSavedPhase = @(MGStartupPhaseName(startup.lastPhase));
    }
}

- (void)start {
    self.sessionID = NSUUID.UUID.UUIDString;
    self.failureJournalStatus = @"not_armed";
    self.queue = dispatch_queue_create("com.morgangame.startup-checkpoint", DISPATCH_QUEUE_SERIAL);
    dispatch_async(self.queue, ^{
        @autoreleasepool {
            NSFileManager *files = NSFileManager.defaultManager;
            NSURL *support = [files URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
            if (!support || ![files createDirectoryAtURL:support withIntermediateDirectories:YES attributes:nil error:nullptr]) return;
            self.checkpointURL = [support URLByAppendingPathComponent:@"MorganFileOpenCheckpoint.txt"];
            self.failureJournalURL = [support URLByAppendingPathComponent:@"MorganFileOpenFailuresV2.log"];
            self.previousFailures = MGReadFailureJournal(self.failureJournalURL);
            NSFileHandle *prior = [NSFileHandle fileHandleForReadingFromURL:self.checkpointURL error:nullptr];
            if (prior) {
                NSData *data = [prior readDataUpToLength:MGCheckpointMaxBytes + 1 error:nullptr];
                [prior closeAndReturnError:nullptr];
                if (data && data.length <= MGCheckpointMaxBytes) {
                    self.previous = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                }
            }

            self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, self.queue);
            if (!self.timer) return;
            dispatch_source_set_timer(self.timer, DISPATCH_TIME_NOW, 200 * NSEC_PER_MSEC, 25 * NSEC_PER_MSEC);
            dispatch_source_set_event_handler(self.timer, ^{
                @autoreleasepool {
                    [self saveSnapshot];
                    if (!self.samplingGameplay && GameStartupTrace().Snapshot().Saw(StartupPhase::FirstTickCompleted)) {
                        self.samplingGameplay = YES;
                        dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC),
                                                  NSEC_PER_SEC, 100 * NSEC_PER_MSEC);
                    }
                }
            });
            dispatch_resume(self.timer);

        }
    });
}

- (void)recordHandoff:(NSString *)note beforeTick:(dispatch_block_t)beforeTick {
    dispatch_async(self.queue, ^{
        @autoreleasepool {
            self.handoffNote = note;
            if (beforeTick) {
                self.failureJournalStatus = MGArmFailureJournal(self.failureJournalURL, self.sessionID)
                    ? @"armed" : @"unavailable";
            }
            [self saveSnapshot];
            if (beforeTick) {
                dispatch_async(dispatch_get_main_queue(), beforeTick);
            } else if (self.timer) {
                self.samplingGameplay = YES;
                dispatch_source_set_timer(self.timer, DISPATCH_TIME_NOW, NSEC_PER_SEC, 100 * NSEC_PER_MSEC);
            }
        }
    });
}

@end

static MGStartupCheckpoint *MGCheckpoint;
typedef NS_ENUM(NSUInteger, MGLauncherState) {
    MGLauncherStateWaitingForWindow,
    MGLauncherStatePresented,
    MGLauncherStatePreparingExternal,
    MGLauncherStateStarted,
};

static void MGRunOnMainThread(dispatch_block_t block) {
    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_async(dispatch_get_main_queue(), block);
    }
}

__attribute__((visibility("default")))
@interface MGLauncherOverlayWindow : UIWindow
@property (nonatomic) BOOL allowsDocumentPickerKey;
@end

@implementation MGLauncherOverlayWindow
@end

__attribute__((visibility("default")))
@interface MGLauncherPresentationCoordinator : NSObject <MGLauncherDelegate>

@property (nonatomic, weak) UIApplication *application;
@property (nonatomic, strong) MGLauncherOverlayWindow *launcherWindow;
@property (nonatomic, weak) UIWindow *gameWindow;
@property (nonatomic, weak) UIWindow *keyWindowBeforePicker;
@property (nonatomic, strong) MGLauncherViewController *launcher;
@property (nonatomic) MGLauncherState state;
@property (nonatomic, copy) NSString *sessionID;
@property (nonatomic, copy) NSString *modeNote;

- (void)startForApplication:(UIApplication *)application;
- (void)presentIfReady;

@end

@implementation MGLauncherPresentationCoordinator

- (void)startForApplication:(UIApplication *)application {
    self.application = application;
    self.state = MGLauncherStateWaitingForWindow;
    self.sessionID = MGCheckpoint.sessionID;
    self.modeNote = @"waiting";

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    for (NSNotificationName name in @[UIWindowDidBecomeVisibleNotification,
                                      UIWindowDidBecomeKeyNotification,
                                      UIApplicationWillEnterForegroundNotification,
                                      UIApplicationDidBecomeActiveNotification]) {
        [center addObserver:self selector:@selector(windowReadinessChanged:) name:name object:nil];
    }
    [self presentIfReady];
}

- (void)windowReadinessChanged:(NSNotification *)notification {
    MGRunOnMainThread(^{
        [self presentIfReady];
    });
}

- (void)presentIfReady {
    UIApplication *application = self.application;
    if (self.state != MGLauncherStateWaitingForWindow || !application ||
        application.applicationState == UIApplicationStateBackground) {
        return;
    }

    id<UIApplicationDelegate> appDelegate = application.delegate;
    Class gtaDelegateClass = NSClassFromString(@"IOSAppDelegate");
    if (!gtaDelegateClass || ![appDelegate isKindOfClass:gtaDelegateClass] ||
        ![appDelegate respondsToSelector:@selector(window)]) {
        return;
    }

    UIWindow *gameWindow = appDelegate.window;
    if (!gameWindow.rootViewController || CGRectIsEmpty(gameWindow.bounds)) {
        return;
    }

    UIWindowScene *scene = gameWindow.windowScene;
    if (scene && scene.activationState != UISceneActivationStateForegroundActive &&
        scene.activationState != UISceneActivationStateForegroundInactive) {
        return;
    }

    GameStartupTrace().Note(StartupPhase::GTAWindowObserved);
    MGLauncherOverlayWindow *window = scene
        ? [[MGLauncherOverlayWindow alloc] initWithWindowScene:scene]
        : [[MGLauncherOverlayWindow alloc] initWithFrame:gameWindow.frame];

    // The launcher owns interface rotation while it is visible.
    window.windowLevel = MAX(gameWindow.windowLevel, UIWindowLevelAlert) + 1;
    MGLauncherViewController *launcher = [[MGLauncherViewController alloc] initWithDelegate:self];
    window.rootViewController = launcher;
    launcher.view.accessibilityViewIsModal = YES;

    self.gameWindow = gameWindow;
    self.launcherWindow = window;
    self.launcher = launcher;
    self.state = MGLauncherStatePresented;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [window makeKeyAndVisible];
    GameStartupTrace().Note(StartupPhase::LauncherPresented);
    NSLog(@"[MorganLauncher] Launcher presented");
}

- (BOOL)beginDocumentPickerForLauncher:(MGLauncherViewController *)launcher {
    if (![NSThread isMainThread] || self.state != MGLauncherStatePresented || launcher != self.launcher ||
        !self.launcherWindow || self.launcherWindow.hidden || self.launcherWindow.allowsDocumentPickerKey) return NO;
    UIWindow *window = self.launcherWindow;
    if (window.isKeyWindow) return YES;
    UIWindow *previous = window.windowScene.keyWindow ?: self.gameWindow;
    if (!previous || previous == window || previous.hidden || previous.windowScene != window.windowScene) return NO;
    self.keyWindowBeforePicker = previous;
    self.launcherWindow.allowsDocumentPickerKey = YES;
    [window makeKeyWindow];
    if (window.isKeyWindow) return YES;
    [self endDocumentPickerForLauncher:launcher];
    return NO;
}

- (void)endDocumentPickerForLauncher:(MGLauncherViewController *)launcher {
    if (![NSThread isMainThread] || launcher != self.launcher || !self.launcherWindow.allowsDocumentPickerKey) return;
    UIWindow *window = self.launcherWindow;
    UIWindow *previous = self.keyWindowBeforePicker;
    self.keyWindowBeforePicker = nil;
    self.launcherWindow.allowsDocumentPickerKey = NO;
    if (!window.isKeyWindow) return;
    if (!previous || previous.hidden || previous.windowScene != window.windowScene) previous = self.gameWindow;
    if (previous && !previous.hidden && previous.windowScene == window.windowScene) {
        [previous makeKeyWindow];
    }
}

- (NSString *)diagnosticsForLauncher:(MGLauncherViewController *)launcher {
    return [NSString stringWithFormat:@"[current launch]\n%@%@\nlast_completed_startup_checkpoint=%@\n"
        "[previous saved checkpoint]\n%@\n[failure-only journal]\ncurrent_status=%@\n%@\n[chat keyboard timings]\n%@",
        MGStartupText(GameStartupTrace().Snapshot()), MGDiagnosticsText(self.sessionID, self.modeNote),
        MGCheckpoint.lastSavedPhase ?: @"none", MGCheckpoint.previous ?: @"(none)",
        MGCheckpoint.failureJournalStatus ?: @"not_armed", MGCheckpoint.previousFailures ?: @"(none)",
        MGKeyboardProbeReport()];
}

- (void)revealGameWithLoadingScreen:(BOOL)showLoading {
    morgan::fileio::CStorage::MarkSampLogGameStarted();
    CNecessary::Log(OBFUSCATE("Launcher handoff to GTA"));
    if (showLoading) {
        MGPrepareLoadingScreen(self.gameWindow.rootViewController);
        CLoadingScreen::SetInterface(true);
        CLoadingScreen::UpdateList("เตรียมเนื้อหาที่จำเป็น...");
    }
    self.state = MGLauncherStateStarted;
    self.launcherWindow.hidden = YES;
    [self.gameWindow makeKeyWindow];
    self.launcherWindow.rootViewController = nil;
    self.launcherWindow = nil;
    self.launcher = nil;
    GameStartupTrace().Note(StartupPhase::HandoffCompleted);
    NSLog(@"[MorganLauncher] Start accepted; original GTA window revealed");
}

- (void)launcher:(MGLauncherViewController *)launcher requestStartForMode:(MGLauncherMode)mode
       completion:(void (^)(NSString * _Nullable error))completion {
    if (![NSThread isMainThread] || self.state != MGLauncherStatePresented || launcher != self.launcher) {
        completion(@"Launcher ยังไม่พร้อมเริ่มเกม");
        return;
    }

    using morgan::fileio::CStorage;
    using morgan::fileio::Mode;
    const bool externalOnly = mode == MGLauncherModeExternalRuntimeTest;
    const bool samp = mode == MGLauncherModeSampTest;
    if (externalOnly != CStorage::IsAssetFreeBundle()) {
        completion(@"โหมดเริ่มเกมไม่ตรงกับชุดที่ติดตั้ง");
        return;
    }
    if (NSString* blocker = CStorage::LaunchBlocker(samp, externalOnly)) {
        self.modeNote = @"blocked: external asset readiness";
        completion(blocker);
        return;
    }
    if (!externalOnly) {
        if (const auto error = CHook::SetGameStreamMode(samp ? Mode::SampReadOnly : Mode::OriginalOnly)) {
            const auto selection = CHook::GameStreamSnapshot().selection;
            if (selection == StreamSelection::EarlyRead || selection == StreamSelection::OriginalOnlyAfterEarlyRead) {
                self.modeNote = @"SAMP timing blocked: a matching read reached the native bridge before selection";
                completion(@"GTA เริ่มเปิดสคริปต์ก่อนเลือกโหมด SAMP สามารถส่งออก Diagnostics หรือเริ่มเกม Offline ได้");
                return;
            }
            self.modeNote = [NSString stringWithFormat:@"SAMP mode rejected (error %d)", error.value()];
            completion([NSString stringWithFormat:@"เปิดโหมด SAMP ไม่สำเร็จ (error %d) สามารถส่งออก Diagnostics หรือเริ่มเกม Offline ได้", error.value()]);
            return;
        }
        if (samp) MGObserveBundledSampMenu();
        self.modeNote = samp ? @"SAMP mode accepted before revealing GTA" : @"Offline mode accepted";
        [self revealGameWithLoadingScreen:samp];
        [MGCheckpoint recordHandoff:self.modeNote beforeTick:nil];
        completion(nil);
        return;
    }

    if (!MGOwnsDeferredTick() || !GameStartupTrace().Snapshot().CanReleaseExternalTick()) {
        self.modeNote = @"blocked: no verified deferred GTA tick";
        completion(@"ยังไม่พบ GTA tick ที่หน่วงไว้ กรุณาส่งออก Diagnostics");
        return;
    }
    self.state = MGLauncherStatePreparingExternal;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool {
            const auto error = CHook::SetGameStreamMode(Mode::ExternalOnly);
            dispatch_async(dispatch_get_main_queue(), ^{
                if (error) {
                    self.state = MGLauncherStatePresented;
                    self.modeNote = [NSString stringWithFormat:@"ExternalOnly preparation failed (error %d)", error.value()];
                    completion([NSString stringWithFormat:@"เตรียม ExternalOnly ไม่สำเร็จ (error %d) กรุณาส่งออก Diagnostics", error.value()]);
                    return;
                }
                GameStartupTrace().Note(StartupPhase::ExternalModeSelected);
                self.modeNote = @"ExternalOnly selected before restoring GTA tick";
                [MGCheckpoint recordHandoff:self.modeNote beforeTick:^{
                    if (CHook::GameStreamSnapshot().selection != StreamSelection::ExternalOnly || !MGReleaseDeferredTick()) {
                        (void)CHook::SetGameStreamMode(Mode::OriginalOnly);
                        self.state = MGLauncherStatePresented;
                        self.modeNote = @"blocked: lost ownership of the deferred GTA tick or ExternalOnly mode";
                        completion(@"คืน GTA tick ไม่ได้ กรุณาส่งออก Diagnostics แล้วเปิดแอปใหม่");
                        return;
                    }
                    GameStartupTrace().Note(StartupPhase::TickGateReleased);
                    self.modeNote = @"ExternalOnly accepted; GTA tick gate released";
                    [self revealGameWithLoadingScreen:YES];
                    completion(nil);
                }];
            });
        }
    });
}

@end

static MGLauncherPresentationCoordinator *MGCoordinator;

void ExitGame()
{
    // Leave the GTA menu callback before cleaning up and terminating on the main queue.
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!MGCoordinator || MGCoordinator.state != MGLauncherStateStarted) return;

        UninitialiseClient();
        std::exit(EXIT_SUCCESS);
    });
}

extern "C" __attribute__((constructor, used, visibility("hidden")))
void MGInstallLauncherRuntime(void) {
    @autoreleasepool {
        MGInstallStartupKeychain();
        GameStartupTrace().Note(StartupPhase::FrameworkRegistered);
        MGCheckpoint = [[MGStartupCheckpoint alloc] init];
        [MGCheckpoint start];
        // UIKit changes run only after the app delegate has completed its launch callback.
        [[NSNotificationCenter defaultCenter]
            addObserverForName:UIApplicationDidFinishLaunchingNotification
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification *notification) {
            MGRunOnMainThread(^{
                if (MGCoordinator || ![notification.object isKindOfClass:[UIApplication class]]) {
                    return;
                }
                GameStartupTrace().Note(StartupPhase::UIKitLaunchFinished);
                InitialiseClient();
                MGInstallAssetFreeTickGate();
                MGCoordinator = [[MGLauncherPresentationCoordinator alloc] init];
                [MGCoordinator startForApplication:(UIApplication *)notification.object];
            });
        }];
        [[NSNotificationCenter defaultCenter]
            addObserverForName:UIApplicationWillTerminateNotification
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification *) {
            if (MGCoordinator) UninitialiseClient();
        }];
        NSLog(@"[MorganLauncher] Runtime entry registered");
    }
}
