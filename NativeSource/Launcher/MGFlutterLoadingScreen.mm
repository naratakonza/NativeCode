#import "MGFlutterLoadingScreen.h"

#import <UIKit/UIKit.h>
#import <dispatch/dispatch.h>

@protocol MGLoadingProject <NSObject>
- (instancetype)initWithPrecompiledDartBundle:(NSBundle*)bundle;
@end

@protocol MGLoadingEngine <NSObject>
- (instancetype)initWithName:(NSString*)name project:(id<MGLoadingProject>)project allowHeadlessExecution:(BOOL)headless;
- (BOOL)runWithEntrypoint:(NSString*)entrypoint;
- (id)binaryMessenger;
- (void)destroyContext;
@end

@protocol MGLoadingController <NSObject>
- (instancetype)initWithEngine:(id<MGLoadingEngine>)engine nibName:(NSString*)nibName bundle:(NSBundle*)bundle;
- (void)setViewOpaque:(BOOL)opaque;
- (void)setSplashScreenView:(UIView*)view;
@end

@protocol MGLoadingCall <NSObject>
- (NSString*)method;
@end

using MGLoadingResult = void (^)(id);
using MGLoadingHandler = void (^)(id<MGLoadingCall>, MGLoadingResult);

@protocol MGLoadingChannel <NSObject>
+ (instancetype)methodChannelWithName:(NSString*)name binaryMessenger:(id)messenger;
- (void)setMethodCallHandler:(MGLoadingHandler)handler;
- (void)invokeMethod:(NSString*)method arguments:(id)arguments;
@end

@interface MGFlutterLoadingScreen : NSObject
@property(nonatomic, strong) id<MGLoadingEngine> engine;
@property(nonatomic, strong) UIViewController<MGLoadingController>* controller;
@property(nonatomic, strong) id<MGLoadingChannel> channel;
@property(nonatomic, copy) NSString* statusText;
@property(nonatomic, strong) NSNumber* percent;
@property(nonatomic) BOOL ready;
@property(nonatomic) BOOL visible;
- (BOOL)attachTo:(UIViewController*)parent;
- (void)close;
- (void)send:(NSString*)method value:(id)value;
@end

static MGFlutterLoadingScreen* loadingScreen;
static NSString* const flutterEngineRevision = @"55eae6864b296dd9f43b2cc7577ec256e5c32a8d";

static void MGRunLoadingOnMain(dispatch_block_t work)
{
    if (NSThread.isMainThread)
    {
        work();
    }
    else
    {
        dispatch_async(dispatch_get_main_queue(), work);
    }
}

@implementation MGFlutterLoadingScreen

- (BOOL)attachTo:(UIViewController*)parent
{
    Class<MGLoadingEngine> engineClass = (Class<MGLoadingEngine>)NSClassFromString(@"FlutterEngine");
    if (!engineClass)
    {
        return NO;
    }

    // Dart AOT must be paired with the Flutter 3.22.1 engine revision.
    NSBundle* flutter = [NSBundle bundleForClass:engineClass];
    if (![[flutter objectForInfoDictionaryKey:@"FlutterEngine"] isEqualToString:flutterEngineRevision])
    {
        return NO;
    }

    NSURL* frameworks = NSBundle.mainBundle.privateFrameworksURL;
    NSBundle* module = [NSBundle bundleWithURL:[frameworks URLByAppendingPathComponent:@"MorganFlutter.framework"]];
    NSURL* assets = [module.resourceURL URLByAppendingPathComponent:@"flutter_assets/AssetManifest.bin"];
    if (!parent.viewIfLoaded.window || ![module.bundleIdentifier isEqualToString:@"com.morgangame.ios.flutter"] ||
        ![[module objectForInfoDictionaryKey:@"MorganFlutterEngine"] isEqualToString:flutterEngineRevision] ||
        ![module.executableURL checkResourceIsReachableAndReturnError:nil] || ![assets checkResourceIsReachableAndReturnError:nil])
    {
        return NO;
    }

    Class<MGLoadingProject> projectClass = (Class<MGLoadingProject>)NSClassFromString(@"FlutterDartProject");
    Class<MGLoadingController> controllerClass = (Class<MGLoadingController>)NSClassFromString(@"FlutterViewController");
    Class<MGLoadingChannel> channelClass = (Class<MGLoadingChannel>)NSClassFromString(@"FlutterMethodChannel");
    if (!projectClass || !controllerClass || !channelClass)
    {
        return NO;
    }

    id<MGLoadingProject> project = [[(id)projectClass alloc] initWithPrecompiledDartBundle:module];
    self.engine = [[(id)engineClass alloc] initWithName:@"MorganFlutter" project:project allowHeadlessExecution:YES];
    if (![self.engine runWithEntrypoint:nil])
    {
        return NO;
    }

    self.channel = [channelClass methodChannelWithName:@"flutter_loadingscreen" binaryMessenger:[self.engine binaryMessenger]];
    self.controller = [[(id)controllerClass alloc] initWithEngine:self.engine nibName:nil bundle:nil];
    if (!self.channel || !self.controller)
    {
        return NO;
    }

    __weak MGFlutterLoadingScreen* weakSelf = self;
    [self.channel setMethodCallHandler:^(id<MGLoadingCall> call, MGLoadingResult result) {
        MGFlutterLoadingScreen* screen = weakSelf;
        if ([call.method isEqualToString:@"ready"] && screen && screen == loadingScreen && !screen.ready)
        {
            screen.ready = YES;
            if (screen.visible)
            {
                [screen send:@"show" value:nil];
                if (screen.statusText)
                {
                    [screen send:@"updateStatusText" value:screen.statusText];
                }
                if (screen.percent)
                {
                    [screen send:@"updateProgress" value:screen.percent];
                }
            }
        }
        else if ([call.method isEqualToString:@"onLoadingComplete"])
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (screen && screen == loadingScreen && !screen.visible)
                {
                    [screen close];
                }
            });
        }
        result(nil);
    }];

    [self.controller setViewOpaque:NO];
    [self.controller setSplashScreenView:nil];
    [parent addChildViewController:self.controller];
    UIView* view = self.controller.view;
    view.userInteractionEnabled = NO;
    view.accessibilityElementsHidden = YES;
    view.backgroundColor = UIColor.clearColor;
    view.frame = parent.view.bounds;
    view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    // The game controller already appeared before the launcher hands control back.
    [self.controller beginAppearanceTransition:YES animated:NO];
    [parent.view addSubview:view];
    [self.controller endAppearanceTransition];
    [self.controller didMoveToParentViewController:parent];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 8 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        MGFlutterLoadingScreen* screen = weakSelf;
        if (screen && screen == loadingScreen && !screen.ready)
        {
            NSLog(@"[MorganLauncher] Flutter loading screen did not initialize");
            [screen close];
        }
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 90 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        MGFlutterLoadingScreen* screen = weakSelf;
        if (screen && screen == loadingScreen && screen.visible)
        {
            NSLog(@"[MorganLauncher] Flutter loading screen timed out");
            [screen close];
        }
    });
    return YES;
}

- (void)send:(NSString*)method value:(id)value
{
    if (self.ready)
    {
        [self.channel invokeMethod:method arguments:value];
    }
}

- (void)close
{
    [self.channel setMethodCallHandler:nil];
    self.channel = nil;
    [self.controller willMoveToParentViewController:nil];
    UIView* view = self.controller.viewIfLoaded;
    const BOOL attached = view.window != nil;
    if (attached)
    {
        [self.controller beginAppearanceTransition:NO animated:NO];
    }
    [view removeFromSuperview];
    if (attached)
    {
        [self.controller endAppearanceTransition];
    }
    [self.controller removeFromParentViewController];
    self.controller = nil;
    [self.engine destroyContext];
    self.engine = nil;
    if (loadingScreen == self)
    {
        loadingScreen = nil;
    }
}

@end

void MGPrepareLoadingScreen(UIViewController* parent)
{
    if (!NSThread.isMainThread || loadingScreen || !parent)
    {
        return;
    }

    loadingScreen = [[MGFlutterLoadingScreen alloc] init];
    if (![loadingScreen attachTo:parent])
    {
        [loadingScreen close];
        NSLog(@"[MorganLauncher] Flutter 3.22.1 loading screen is unavailable");
    }
}

void MGSetLoadingScreenVisible(bool visible)
{
    MGRunLoadingOnMain(^{
        MGFlutterLoadingScreen* screen = loadingScreen;
        if (!screen || screen.visible == visible)
        {
            return;
        }
        screen.visible = visible;
        [screen send:visible ? @"show" : @"hide" value:nil];
        if (!visible)
        {
            __weak MGFlutterLoadingScreen* weakScreen = screen;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                MGFlutterLoadingScreen* pending = weakScreen;
                if (pending && pending == loadingScreen && !pending.visible)
                {
                    [pending close];
                }
            });
        }
    });
}

void MGUpdateLoadingScreenPercent(int percent)
{
    MGRunLoadingOnMain(^{
        MGFlutterLoadingScreen* screen = loadingScreen;
        screen.percent = @(percent);
        [screen send:@"updateProgress" value:screen.percent];
    });
}

void MGUpdateLoadingScreenList(const char* text)
{
    NSString* status = text ? [NSString stringWithUTF8String:text] : nil;
    if (!status)
    {
        return;
    }

    MGRunLoadingOnMain(^{
        MGFlutterLoadingScreen* screen = loadingScreen;
        if (screen && ![screen.statusText isEqualToString:status])
        {
            screen.statusText = status;
            [screen send:@"updateStatusText" value:status];
        }
    });
}
