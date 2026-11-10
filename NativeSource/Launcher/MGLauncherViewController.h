#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class MGLauncherViewController;

typedef NS_ENUM(NSUInteger, MGLauncherMode) {
    MGLauncherModeOffline,
    MGLauncherModeSampTest,
    MGLauncherModeExternalRuntimeTest,
};

@protocol MGLauncherDelegate <NSObject>

// Complete on the main thread with a user-visible error, or nil after handoff.
- (void)launcher:(MGLauncherViewController *)launcher requestStartForMode:(MGLauncherMode)mode
       completion:(void (^)(NSString * _Nullable error))completion;
- (NSString *)diagnosticsForLauncher:(MGLauncherViewController *)launcher;
// The coordinator owns the window and restores the game's key window after Files.
- (BOOL)beginDocumentPickerForLauncher:(MGLauncherViewController *)launcher;
- (void)endDocumentPickerForLauncher:(MGLauncherViewController *)launcher;

@end

__attribute__((visibility("default")))
@interface MGLauncherViewController : UIViewController

// The owning app retains the delegate and performs the actual transition.
- (instancetype)initWithDelegate:(id<MGLauncherDelegate>)delegate NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithNibName:(nullable NSString *)nibNameOrNil
                         bundle:(nullable NSBundle *)nibBundleOrNil NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
