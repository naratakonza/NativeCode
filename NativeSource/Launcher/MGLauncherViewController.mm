#import "MGLauncherViewController.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "../source/game_morgan/CSetting.h"
#include "../source/utils/Storage/CStorage.h"
#include "../source/utils/FileIO/FileOpenDiagnostics.h"

using morgan::fileio::CStorage;
using morgan::fileio::ImportStatus;

static NSString* MGImportStatusName(ImportStatus status) {
    switch (status) {
        case ImportStatus::NotImported: return @"Not Imported";
        case ImportStatus::Validating: return @"Validating";
        case ImportStatus::Ready: return @"Ready";
        case ImportStatus::Failed: return @"Failed";
    }
    return @"Unknown";
}

static NSString* const MGPickerDiagnosticsKey = @"MorganLastDocumentPickerEvents";

@interface MGLauncherViewController () <UIDocumentPickerDelegate, UIAdaptivePresentationControllerDelegate>
@end

static UIButton *MGLauncherButton(NSString *title, NSString *identifier, id target, SEL action) {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    button.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    button.titleLabel.adjustsFontForContentSizeCategory = YES;
    button.titleLabel.numberOfLines = 0;
    button.titleLabel.textAlignment = NSTextAlignmentCenter;
    button.accessibilityIdentifier = identifier;
    [button addTarget:target action:action forControlEvents:UIControlEventTouchUpInside];
    [button.heightAnchor constraintGreaterThanOrEqualToConstant:44].active = YES;
    return button;
}

@implementation MGLauncherViewController {
    __weak id<MGLauncherDelegate> _startDelegate;
    BOOL _startRequested;
    BOOL _assetWorkInProgress;
    BOOL _finishingPicker;
    UIDocumentPickerViewController* _documentPicker;
    UILabel* _assetStatus;
    UIProgressView* _assetProgress;
    UIButton* _importButton;
    UIButton* _cancelImportButton;
    std::shared_ptr<std::atomic<bool>> _importCancellation;
    UIButton* _offlineButton;
    UIButton* _sampButton;
    dispatch_queue_t _assetQueue;
}

- (instancetype)initWithDelegate:(id<MGLauncherDelegate>)delegate {
    self = [super initWithNibName:nil bundle:nil];
    if (self) {
        _startDelegate = delegate;
    }
    return self;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskLandscape;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    return UIInterfaceOrientationLandscapeRight;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self setNeedsUpdateOfSupportedInterfaceOrientations];
    UIWindowScene* scene = self.view.window.windowScene;
    if (scene && !UIInterfaceOrientationIsLandscape(scene.interfaceOrientation)) {
        UIWindowSceneGeometryPreferencesIOS* landscape = [[UIWindowSceneGeometryPreferencesIOS alloc]
            initWithInterfaceOrientations:UIInterfaceOrientationMaskLandscape];
        [scene requestGeometryUpdateWithPreferences:landscape errorHandler:^(NSError* error) {
            NSLog(@"[MorganLauncher] Landscape request failed: %@", error.localizedDescription);
        }];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.blackColor;

    UILabel *title = [[UILabel alloc] init];
    title.text = @"MORGAN GAME";
    title.textColor = UIColor.whiteColor;
    title.textAlignment = NSTextAlignmentCenter;
    title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleTitle1];
    title.adjustsFontForContentSizeCategory = YES;
    title.numberOfLines = 0;
    title.accessibilityTraits |= UIAccessibilityTraitHeader;

    _offlineButton = MGLauncherButton(@"เริ่มเกม Offline", @"morgan.launcher.start", self, @selector(handleOffline:));
    const bool externalOnly = CStorage::IsAssetFreeBundle();
    _offlineButton.hidden = externalOnly;
    _sampButton = MGLauncherButton(externalOnly ? @"เริ่มเกม" : @"ทดสอบ Multiplayer / SAMP",
                                   externalOnly ? @"morgan.launcher.external-runtime" : @"morgan.launcher.samp",
                                   self, @selector(handleSamp:));
    UIButton *exportButton = MGLauncherButton(@"Export Diagnostics", @"morgan.launcher.diagnostics", self, @selector(exportDiagnostics:));

    _importButton = MGLauncherButton(@"นำเข้า Game Assets", @"morgan.launcher.import", self, @selector(importAssets:));
    _cancelImportButton = MGLauncherButton(@"ยกเลิกการนำเข้า", @"morgan.launcher.import.cancel", self, @selector(cancelAssetImport:));
    _cancelImportButton.hidden = YES;
    _assetStatus = [[UILabel alloc] init];
    _assetStatus.textColor = UIColor.whiteColor;
    _assetStatus.textAlignment = NSTextAlignmentCenter;
    _assetStatus.numberOfLines = 0;
    _assetStatus.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    _assetStatus.adjustsFontForContentSizeCategory = YES;
    _assetStatus.accessibilityIdentifier = @"morgan.launcher.assets.status";
    _assetProgress = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    _assetProgress.accessibilityIdentifier = @"morgan.launcher.assets.progress";

    UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:
        @[title, _offlineButton, _sampButton, _importButton, _assetStatus, _assetProgress, _cancelImportButton, exportButton]];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 8;

    UIScrollView* scroll = [[UIScrollView alloc] init];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:scroll];
    [scroll addSubview:stack];

    UILayoutGuide *safeArea = self.view.safeAreaLayoutGuide;
    NSLayoutConstraint* center = [stack.centerYAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.centerYAnchor];
    center.priority = UILayoutPriorityDefaultLow;
    NSLayoutConstraint* top = [stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:12];
    top.priority = UILayoutPriorityDefaultLow - 1;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.topAnchor constraintEqualToAnchor:safeArea.topAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:safeArea.bottomAnchor],
        [scroll.leadingAnchor constraintEqualToAnchor:safeArea.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:safeArea.trailingAnchor],
        [stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:24],
        [stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-24],
        [stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-48],
        [stack.topAnchor constraintGreaterThanOrEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:12],
        [stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-12],
        center,
        top,
    ]];

    _assetQueue = dispatch_queue_create("com.morgangame.asset-import", DISPATCH_QUEUE_SERIAL);
    [self setAssetWorkInProgress:YES];
    __weak MGLauncherViewController* weakSelf = self;
    dispatch_async(_assetQueue, ^{
        GameStartupTrace().Note(StartupPhase::StorageValidationStarted);
        CStorage::ValidateExternalAssets();
        (void)CStorage::GetAssetStatus();
        GameStartupTrace().Note(StartupPhase::StorageValidationFinished);
        dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf setAssetWorkInProgress:NO]; });
    });
}

- (void)setAssetWorkInProgress:(BOOL)busy {
    _assetWorkInProgress = busy;
    _importButton.enabled = !busy;
    _offlineButton.enabled = !busy;
    _sampButton.enabled = !busy;
    _assetProgress.hidden = !busy;
    _cancelImportButton.hidden = !busy || !_importCancellation;
    if (busy) {
        _assetProgress.progress = 0;
        _assetStatus.text = @"กำลังตรวจ Game Assets…";
        return;
    }
    const auto assets = CStorage::GetAssetStatus();
    NSString* size = [NSByteCountFormatter stringFromByteCount:static_cast<long long>(assets.importedBytes)
                                                   countStyle:NSByteCountFormatterCountStyleFile];
    const bool externalOnly = CStorage::IsAssetFreeBundle();
    NSString* readiness = CStorage::LaunchBlocker(true, externalOnly);
    if (externalOnly) {
        _sampButton.enabled = readiness == nil;
        if (!readiness) {
            _assetStatus.text = @"Game Assets พร้อมแล้ว";
            return;
        }
    }
    NSString* headline = MGImportStatusName(assets.importStatus);
    if (!externalOnly && assets.importStatus == ImportStatus::Ready && assets.externalReady) {
        headline = @"Ready — External (SAMP)";
    }
    NSMutableArray<NSString*>* rows = [NSMutableArray array];
    for (std::size_t i = 0; i < assets.groups.size(); ++i) {
        const auto& group = assets.groups[i];
        NSString* state = group.present && !group.valid ? @"Failed" :
            group.valid ? (externalOnly ? @"Imported — รอทดสอบการอ่านจริง"
                                        : i == 0 ? @"Ready — External" : @"Imported — Loader coverage unverified") :
            assets.bundleGroups[i] ? @"Bundle" : @"Not Installed";
        [rows addObject:[NSString stringWithFormat:@"%s · %@", morgan::fileio::storage::GroupNames[i], state]];
    }
    if (!readiness) readiness = @"เริ่มเกม · พร้อมตรวจโหมด";
    _assetStatus.text = [NSString stringWithFormat:@"Assets: %@ · %@\n%@\n%@", headline, size, readiness,
                                                   [rows componentsJoinedByString:@"\n"]];
}

- (void)cancelAssetImport:(UIButton*)sender {
    (void)sender;
    if (!_importCancellation) return;
    _importCancellation->store(true);
    _cancelImportButton.enabled = NO;
    _assetStatus.text = @"กำลังยกเลิกการนำเข้า Game Assets…";
}

- (void)recordPickerEvent:(NSString*)event {
    NSAssert(NSThread.isMainThread, @"Picker events are serialized on the main thread");
    NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
    NSMutableArray<NSString*>* events = [[defaults stringArrayForKey:MGPickerDiagnosticsKey] mutableCopy];
    if (!events) events = [NSMutableArray array];
    [events addObject:event];
    [defaults setObject:events forKey:MGPickerDiagnosticsKey];
    NSLog(@"[MorganAssetPicker] %@", event);
}

- (void)showImportFailure:(NSString*)message {
    [self recordPickerEvent:@"import_failed"];
    [self setAssetWorkInProgress:NO];
    _assetStatus.text = [NSString stringWithFormat:@"%@\n%@", _assetStatus.text, message];
    if (!self.presentedViewController) [self showError:message];
}

- (void)importAssets:(UIButton*)sender {
    (void)sender;
    NSAssert(NSThread.isMainThread, @"Present Files from the main thread");
    if (_assetWorkInProgress || _startRequested || _documentPicker || self.presentedViewController) return;
    [NSUserDefaults.standardUserDefaults removeObjectForKey:MGPickerDiagnosticsKey];
    [self recordPickerEvent:@"picker_requested"];
    id<MGLauncherDelegate> delegate = _startDelegate;
    if (!self.view.window || ![delegate beginDocumentPickerForLauncher:self]) {
        [self recordPickerEvent:@"picker_presentation_failed"];
        [self showImportFailure:@"ไม่สามารถเปิด Files ได้ กรุณาลองอีกครั้ง"];
        return;
    }

    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc]
        initForOpeningContentTypes:@[UTTypeZIP] asCopy:YES];
    picker.delegate = self;
    picker.allowsMultipleSelection = NO;
    picker.modalPresentationStyle = UIModalPresentationFullScreen;
    picker.presentationController.delegate = self;
    _documentPicker = picker;
    [self recordPickerEvent:@"copy_requested=UIKit"];
    [self recordPickerEvent:@"copy_started=not_observable_in_UIKit"];
    [self presentViewController:picker animated:YES completion:^{
        if (self->_documentPicker == picker && !self->_finishingPicker) {
            [self recordPickerEvent:@"picker_presented"];
        }
    }];
}

- (void)finishPicker:(UIDocumentPickerViewController*)picker completion:(dispatch_block_t)completion {
    NSAssert(NSThread.isMainThread, @"Dismiss Files on the main thread");
    if (_documentPicker != picker || _finishingPicker) return;
    _finishingPicker = YES;
    dispatch_block_t finish = ^{
        if (self->_documentPicker != picker) return;
        picker.delegate = nil;
        self->_documentPicker = nil;
        self->_finishingPicker = NO;
        [self->_startDelegate endDocumentPickerForLauncher:self];
        [self recordPickerEvent:@"picker_dismissed"];
        completion();
    };
    id<UIViewControllerTransitionCoordinator> transition = picker.transitionCoordinator;
    if (picker.isBeingDismissed && [transition animateAlongsideTransition:nil
              completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        if (context.isCancelled && picker.presentingViewController) {
            [picker dismissViewControllerAnimated:YES completion:finish];
        } else {
            finish();
        }
    }]) return;
    if (picker.presentingViewController) {
        [picker dismissViewControllerAnimated:YES completion:finish];
    } else {
        finish();
    }
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)picker {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ [self documentPickerWasCancelled:picker]; });
        return;
    }
    if (_documentPicker != picker || _finishingPicker) return;
    [self recordPickerEvent:@"picker_cancelled"];
    [self finishPicker:picker completion:^{ [self setAssetWorkInProgress:NO]; }];
}

- (void)presentationControllerDidDismiss:(UIPresentationController*)controller {
    UIDocumentPickerViewController* picker = _documentPicker;
    if (picker && controller.presentedViewController == picker) [self documentPickerWasCancelled:picker];
}

- (void)documentPicker:(UIDocumentPickerViewController*)picker didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ [self documentPicker:picker didPickDocumentsAtURLs:urls]; });
        return;
    }
    if (_documentPicker != picker || _finishingPicker) return;

    [self recordPickerEvent:@"url_selected"];
    NSURL* url = urls.firstObject;
    NSString* extension = url.pathExtension;
    const BOOL valid = urls.count == 1 && url.isFileURL && extension.length > 0 &&
        [extension caseInsensitiveCompare:@"zip"] == NSOrderedSame;
    if (valid) {
        [self recordPickerEvent:@"copy_received=UIKit"];
        _importCancellation = std::make_shared<std::atomic<bool>>(false);
        _cancelImportButton.enabled = YES;
        [self setAssetWorkInProgress:YES];
        _assetStatus.text = @"กำลังเตรียมนำเข้า Game Assets…";
    }
    [self finishPicker:picker completion:^{
        if (!valid) {
            [self showImportFailure:@"เลือกไฟล์ .zip หนึ่งไฟล์จากแอป Files"];
            return;
        }
        [self importPickedZip:url];
    }];
}

- (void)importPickedZip:(NSURL*)url {
    [self recordPickerEvent:@"import_queued"];
    const auto cancellation = _importCancellation;
    __weak MGLauncherViewController* weakSelf = self;
    dispatch_async(_assetQueue, ^{
        @autoreleasepool {
            GameStartupTrace().Note(StartupPhase::ImportStarted);
            __block BOOL copyingAssets = NO;
            NSString* error = CStorage::ImportZip(url, ^(double fraction) {
                const BOOL firstChunk = !copyingAssets;
                copyingAssets = YES;
                dispatch_async(dispatch_get_main_queue(), ^{
                    MGLauncherViewController* launcher = weakSelf;
                    if (launcher && launcher->_assetWorkInProgress) {
                        if (firstChunk) [launcher recordPickerEvent:@"asset_copy_started"];
                        [launcher->_assetProgress setProgress:static_cast<float>(fraction) animated:YES];
                    }
                });
            }, ^{
                dispatch_async(dispatch_get_main_queue(), ^{
                    MGLauncherViewController* launcher = weakSelf;
                    [launcher recordPickerEvent:@"access_granted"];
                    [launcher recordPickerEvent:@"import_started"];
                });
            }, ^bool {
                return cancellation->load();
            });
            GameStartupTrace().Note(error ? StartupPhase::ImportFailed : StartupPhase::ImportCompleted);
            dispatch_async(dispatch_get_main_queue(), ^{
                MGLauncherViewController* launcher = weakSelf;
                if (!launcher) return;
                launcher->_importCancellation.reset();
                if (error) {
                    if (cancellation->load()) [launcher recordPickerEvent:@"import_cancelled"];
                    [launcher showImportFailure:[@"นำเข้า Assets ไม่สำเร็จ\n" stringByAppendingString:error]];
                } else {
                    if (auto* settings = CSetting::GetConfig()) settings->LoadSettings();
                    [launcher recordPickerEvent:@"import_completed"];
                    [launcher setAssetWorkInProgress:NO];
                }
            });
        }
    });
}

- (void)showError:(NSString *)message {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Morgan Game"
        message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"ตกลง" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)requestStart:(MGLauncherMode)mode {
    id<MGLauncherDelegate> delegate = _startDelegate;
    if (_assetWorkInProgress || _startRequested || !delegate || self.presentedViewController) return;

    _startRequested = YES;
    self.view.userInteractionEnabled = NO;
    if (mode == MGLauncherModeExternalRuntimeTest) _assetStatus.text = @"กำลังเริ่ม GTA…";
    [delegate launcher:self requestStartForMode:mode completion:^(NSString *error) {
        if (!error) return;
        self->_startRequested = NO;
        self.view.userInteractionEnabled = YES;
        [self setAssetWorkInProgress:NO];
        [self showError:error];
    }];
}

- (void)handleOffline:(UIButton *)sender {
    [self requestStart:MGLauncherModeOffline];
}

- (void)handleSamp:(UIButton *)sender {
    [self requestStart:CStorage::IsAssetFreeBundle() ? MGLauncherModeExternalRuntimeTest : MGLauncherModeSampTest];
}

- (void)exportDiagnostics:(UIButton *)sender {
    id<MGLauncherDelegate> delegate = _startDelegate;
    if (_assetWorkInProgress || _startRequested || !delegate || self.presentedViewController) return;

    NSString* identifier = NSUUID.UUID.UUIDString;
    NSURL* temporaryRoot = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSURL* directory = [temporaryRoot URLByAppendingPathComponent:identifier isDirectory:YES];
    NSString* name = [NSString stringWithFormat:@"Morgan-FileOpen-%@.txt", identifier];
    NSURL* url = [directory URLByAppendingPathComponent:name];
    NSURL* logURL = [directory URLByAppendingPathComponent:[NSString stringWithFormat:@"Morgan-%@.log", identifier]];
    NSArray<NSString*>* events = [NSUserDefaults.standardUserDefaults stringArrayForKey:MGPickerDiagnosticsKey];
    NSString* pickerDiagnostics = events.count ? [events componentsJoinedByString:@"\n"] : @"(none)";
    NSString* report = [NSString stringWithFormat:@"%@\n[last document picker attempt]\n"
        "picker_callback_received=%d\nzip_copy_completed=%d\n%@\n",
        [delegate diagnosticsForLauncher:self], static_cast<int>([events containsObject:@"url_selected"]),
        static_cast<int>([events containsObject:@"copy_received=UIKit"]), pickerDiagnostics];
    self.view.userInteractionEnabled = NO;
    dispatch_async(_assetQueue, ^{
        NSFileManager* files = NSFileManager.defaultManager;
        const BOOL created = [files createDirectoryAtURL:directory withIntermediateDirectories:NO attributes:nil error:nullptr];
        const BOOL logCopied = created && CStorage::CopySampLog(logURL);
        NSString* exportStatus = logCopied ? @"current_launch" : @"current_launch_log_unavailable";
        NSString* exportReport = [report stringByAppendingFormat:@"\n[log export]\nmorgan_log=%@\n", exportStatus];
        const BOOL reportWritten = created && [exportReport writeToURL:url atomically:YES encoding:NSUTF8StringEncoding error:nullptr];
        if (created && !reportWritten) [files removeItemAtURL:directory error:nullptr];

        dispatch_async(dispatch_get_main_queue(), ^{
            self.view.userInteractionEnabled = YES;
            if (!self.view.window || self.presentedViewController) {
                if (reportWritten) [files removeItemAtURL:directory error:nullptr];
                return;
            }
            if (!reportWritten) {
                [self showError:@"ไม่สามารถสร้างไฟล์ Diagnostics ได้"];
                return;
            }

            NSArray<NSURL*>* items = logCopied ? @[url, logURL] : @[url];
            UIActivityViewController* share = [[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:nil];
            share.popoverPresentationController.sourceView = sender;
            share.popoverPresentationController.sourceRect = sender.bounds;
            share.completionWithItemsHandler = ^(UIActivityType activity, BOOL completed, NSArray* sharedItems, NSError* error) {
                [files removeItemAtURL:directory error:nullptr];
            };
            [self presentViewController:share animated:YES completion:nil];
        });
    });
}

@end
