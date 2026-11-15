#import "MGFlutter.h"

#import <UIKit/UIKit.h>
#import <dispatch/dispatch.h>

#include "../Main.h"
#include "../source/game_morgan/CTimer.h"
#include "../source/game_morgan/Multiplayer/Guis/CGui.h"

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
- (id)arguments;
@end

using MGLoadingResult = void (^)(id);
using MGLoadingHandler = void (^)(id<MGLoadingCall>, MGLoadingResult);

@protocol MGLoadingChannel <NSObject>
+ (instancetype)methodChannelWithName:(NSString*)name binaryMessenger:(id)messenger;
- (void)setMethodCallHandler:(MGLoadingHandler)handler;
- (void)invokeMethod:(NSString*)method arguments:(id)arguments;
@end

@interface MGFlutterOverlayView : UIView
@property(nonatomic) CGRect chatArea;
@property(nonatomic) CGRect spawnArea;
@end

@implementation MGFlutterOverlayView
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent*)event
{
    return [super pointInside:point withEvent:event] &&
        (CGRectContainsPoint(self.chatArea, point) || CGRectContainsPoint(self.spawnArea, point));
}
@end

@interface MGFlutter : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic, strong) id<MGLoadingEngine> engine;
@property(nonatomic, strong) UIViewController<MGLoadingController>* controller;
@property(nonatomic, strong) id<MGLoadingChannel> channel;
@property(nonatomic, strong) id<MGLoadingChannel> chatChannel;
@property(nonatomic, strong) id<MGLoadingChannel> spawnChannel;
@property(nonatomic, strong) MGFlutterOverlayView* overlay;
@property(nonatomic, strong) UITapGestureRecognizer* outsideChatTap;
@property(nonatomic, strong) NSMutableArray<NSDictionary*>* pendingMessages;
@property(nonatomic, copy) NSString* statusText;
@property(nonatomic, strong) NSNumber* percent;
@property(nonatomic) BOOL ready;
@property(nonatomic) BOOL visible;
@property(nonatomic) BOOL chatReady;
@property(nonatomic) BOOL chatVisible;
@property(nonatomic) BOOL spawnReady;
@property(nonatomic) BOOL spawnPresented;
@property(nonatomic) BOOL paused;
@property(nonatomic) BOOL loadingFinished;
@property(nonatomic, strong) NSDictionary* chatLayout;
- (BOOL)attachTo:(UIViewController*)parent;
- (void)close;
- (void)send:(NSString*)method value:(id)value;
- (void)updateOverlayVisibility;
- (void)updateSpawnVisibility;
- (BOOL)isChatInteractive;
@end

static MGFlutter* flutter;
static bool spawnVisible;
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

static NSString* MGChatColor(uint32_t rgba)
{
    return [NSString stringWithFormat:@"{%06X}", static_cast<unsigned>(rgba >> 8)];
}

static CGRect MGFlutterHitRegion(NSDictionary* args, CGRect bounds)
{
    if (![args[@"x"] isKindOfClass:NSNumber.class] || ![args[@"y"] isKindOfClass:NSNumber.class] ||
        ![args[@"width"] isKindOfClass:NSNumber.class] || ![args[@"height"] isKindOfClass:NSNumber.class])
    {
        return CGRectZero;
    }

    const auto x = [args[@"x"] doubleValue];
    const auto y = [args[@"y"] doubleValue];
    const auto width = [args[@"width"] doubleValue];
    const auto height = [args[@"height"] doubleValue];
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0)
    {
        return CGRectZero;
    }
    return CGRectIntersection(CGRectMake(x, y, width, height), bounds);
}

static bool MGHandleSpawnAction(NSString* action)
{
    CSpawnScreen* screen = CGui::GetSpawnScreen();
    if (!screen)
    {
        return false;
    }
    if ([action isEqualToString:@"PREV_SKIN"])
    {
        return screen->HandleAction(eSpawnAction::PREV_SKIN);
    }
    if ([action isEqualToString:@"NEXT_SKIN"])
    {
        return screen->HandleAction(eSpawnAction::NEXT_SKIN);
    }
    if ([action isEqualToString:@"SPAWN_PLAYER"])
    {
        return screen->HandleAction(eSpawnAction::SPAWN_PLAYER);
    }
    return false;
}

@implementation MGFlutter

- (BOOL)attachTo:(UIViewController*)parent
{
    Class<MGLoadingEngine> engineClass = (Class<MGLoadingEngine>)NSClassFromString(@"FlutterEngine");
    if (!engineClass)
    {
        return NO;
    }

    // Dart AOT must be paired with the Flutter 3.22.1 engine revision.
    NSBundle* engineBundle = [NSBundle bundleForClass:engineClass];
    if (![[engineBundle objectForInfoDictionaryKey:@"FlutterEngine"] isEqualToString:flutterEngineRevision])
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
    self.chatChannel = [channelClass methodChannelWithName:@"flutter_chat" binaryMessenger:[self.engine binaryMessenger]];
    self.spawnChannel = [channelClass methodChannelWithName:@"flutter_spawn" binaryMessenger:[self.engine binaryMessenger]];
    self.pendingMessages = [NSMutableArray array];
    self.controller = [[(id)controllerClass alloc] initWithEngine:self.engine nibName:nil bundle:nil];
    if (!self.channel || !self.chatChannel || !self.spawnChannel || !self.controller)
    {
        return NO;
    }

    __weak MGFlutter* weakSelf = self;
    [self.channel setMethodCallHandler:^(id<MGLoadingCall> call, MGLoadingResult result) {
        MGFlutter* screen = weakSelf;
        if ([call.method isEqualToString:@"ready"] && screen && screen == flutter && !screen.ready)
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
                if (screen && screen == flutter && !screen.visible)
                {
                    screen.loadingFinished = YES;
                    [screen updateOverlayVisibility];
                }
            });
        }
        result(nil);
    }];
    [self.chatChannel setMethodCallHandler:^(id<MGLoadingCall> call, MGLoadingResult result) {
        MGFlutter* screen = weakSelf;
        if (!screen || screen != flutter) {
            result(@NO);
            return;
        }

        if ([call.method isEqualToString:@"ready"])
        {
            screen.chatReady = YES;
            screen.paused = CTimer::IsUserPause();
            for (NSDictionary* message in screen.pendingMessages) {
                [screen.chatChannel invokeMethod:@"addMessage" arguments:message];
            }
            [screen.pendingMessages removeAllObjects];
            if (screen.chatLayout) {
                [screen.chatChannel invokeMethod:@"ChatPosition" arguments:screen.chatLayout];
            }
            [screen.chatChannel invokeMethod:@"setPauseState" arguments:@(screen.paused)];
            [screen updateOverlayVisibility];
            result(@YES);
            return;
        }

        NSDictionary* args = [call.arguments isKindOfClass:NSDictionary.class] ? call.arguments : nil;
        if ([call.method isEqualToString:@"sendText"] && [screen isChatInteractive])
        {
            NSString* text = [args[@"text"] isKindOfClass:NSString.class] ? args[@"text"] : nil;
            static const NSStringEncoding thaiEncoding = CFStringConvertEncodingToNSStringEncoding(CFStringConvertIANACharSetNameToEncoding(CFSTR("windows-874")));
            NSData* encoded = [text dataUsingEncoding:thaiEncoding] ?: [text dataUsingEncoding:NSUTF8StringEncoding];
            const bool queued = encoded.length && CChat::QueueInput(std::string(static_cast<const char*>(encoded.bytes), encoded.length));
            result(@(queued));
            return;
        }
        if ([call.method isEqualToString:@"inputActive"] && [call.arguments isKindOfClass:NSNumber.class])
        {
            screen.outsideChatTap.enabled = [screen isChatInteractive] && [call.arguments boolValue];
            result(@YES);
            return;
        }
        if ([call.method isEqualToString:@"hitRegion"])
        {
            screen.overlay.chatArea = [screen isChatInteractive] ? MGFlutterHitRegion(args, screen.overlay.bounds) : CGRectZero;
            result(@YES);
            return;
        }
        result(@NO);
    }];
    [self.spawnChannel setMethodCallHandler:^(id<MGLoadingCall> call, MGLoadingResult result) {
        MGRunLoadingOnMain(^{
            MGFlutter* screen = weakSelf;
            if (!screen || screen != flutter)
            {
                result(@NO);
                return;
            }
            if ([call.method isEqualToString:@"ready"])
            {
                screen.spawnReady = YES;
                [screen updateSpawnVisibility];
                result(@YES);
                return;
            }
            if (!screen.spawnPresented)
            {
                result(@NO);
                return;
            }

            NSDictionary* args = [call.arguments isKindOfClass:NSDictionary.class] ? call.arguments : nil;
            if ([call.method isEqualToString:@"hitRegion"])
            {
                screen.overlay.spawnArea = MGFlutterHitRegion(args, screen.overlay.bounds);
                result(@YES);
                return;
            }
            if ([call.method isEqualToString:@"onSpawnAction"])
            {
                NSString* action = [args[@"action"] isKindOfClass:NSString.class] ? args[@"action"] : nil;
                result(@(MGHandleSpawnAction(action)));
                return;
            }
            result(@NO);
        });
    }];

    [self.controller setViewOpaque:NO];
    [self.controller setSplashScreenView:nil];
    [parent addChildViewController:self.controller];
    UIView* view = self.controller.view;
    view.userInteractionEnabled = YES;
    view.accessibilityElementsHidden = YES;
    view.backgroundColor = UIColor.clearColor;
    view.frame = parent.view.bounds;
    view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.overlay = [[MGFlutterOverlayView alloc] initWithFrame:parent.view.bounds];
    self.overlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    view.frame = self.overlay.bounds;
    // The game controller already appeared before the launcher hands control back.
    [self.controller beginAppearanceTransition:YES animated:NO];
    [parent.view addSubview:self.overlay];
    [self.overlay addSubview:view];
    [self.controller endAppearanceTransition];
    [self.controller didMoveToParentViewController:parent];

    self.outsideChatTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleOutsideChatTap:)];
    self.outsideChatTap.delegate = self;
    self.outsideChatTap.enabled = NO;
    self.outsideChatTap.cancelsTouchesInView = NO;
    self.outsideChatTap.delaysTouchesBegan = NO;
    self.outsideChatTap.delaysTouchesEnded = NO;
    [parent.view addGestureRecognizer:self.outsideChatTap];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 8 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        MGFlutter* screen = weakSelf;
        if (screen && screen == flutter && !screen.ready)
        {
            NSLog(@"[MorganLauncher] Flutter loading screen did not initialize");
            [screen close];
        }
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 90 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        MGFlutter* screen = weakSelf;
        if (screen && screen == flutter && screen.visible)
        {
            NSLog(@"[MorganLauncher] Flutter loading screen timed out");
            MGSetLoadingScreenVisible(false);
        }
    });
    return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)recognizer shouldReceiveTouch:(UITouch*)touch
{
    return recognizer == self.outsideChatTap && [self isChatInteractive] &&
        !CGRectContainsPoint(self.overlay.chatArea, [touch locationInView:self.overlay]);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)recognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)other
{
    return recognizer == self.outsideChatTap || other == self.outsideChatTap;
}

- (void)handleOutsideChatTap:(UITapGestureRecognizer*)recognizer
{
    if (recognizer.state == UIGestureRecognizerStateRecognized && [self isChatInteractive])
    {
        [self.chatChannel invokeMethod:@"HideInputChat" arguments:nil];
    }
}

- (void)send:(NSString*)method value:(id)value
{
    if (self.ready)
    {
        [self.channel invokeMethod:method arguments:value];
    }
}

- (BOOL)isChatInteractive
{
    return self.chatVisible && self.loadingFinished && !self.visible && !self.paused;
}

- (void)updateOverlayVisibility
{
    [self updateSpawnVisibility];
    const BOOL interactive = [self isChatInteractive];
    if (!interactive) {
        self.overlay.chatArea = CGRectZero;
        self.outsideChatTap.enabled = NO;
    }
    if (self.chatReady) {
        const BOOL show = self.chatVisible && self.loadingFinished && !self.visible;
        [self.chatChannel invokeMethod:show ? @"show" : @"hide" arguments:nil];
    }
}

- (void)updateSpawnVisibility
{
    const BOOL show = self.spawnReady && spawnVisible && self.loadingFinished && !self.visible && !self.paused;
    self.controller.viewIfLoaded.accessibilityElementsHidden = ![self isChatInteractive] && !show;
    if (self.spawnPresented != show)
    {
        self.spawnPresented = show;
        self.overlay.spawnArea = CGRectZero;
        [self.spawnChannel invokeMethod:@"toggleSpawnMenu" arguments:@(show)];
    }
}

- (void)close
{
    [self.outsideChatTap.view removeGestureRecognizer:self.outsideChatTap];
    self.outsideChatTap = nil;
    [self.spawnChannel setMethodCallHandler:nil];
    self.spawnChannel = nil;
    [self.chatChannel setMethodCallHandler:nil];
    self.chatChannel = nil;
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
    [self.overlay removeFromSuperview];
    self.overlay = nil;
    if (attached)
    {
        [self.controller endAppearanceTransition];
    }
    [self.controller removeFromParentViewController];
    self.controller = nil;
    [self.engine destroyContext];
    self.engine = nil;
    if (flutter == self)
    {
        flutter = nil;
    }
}

@end

void MGPrepareLoadingScreen(UIViewController* parent)
{
    if (!NSThread.isMainThread || flutter || !parent)
    {
        return;
    }

    flutter = [[MGFlutter alloc] init];
    if (![flutter attachTo:parent])
    {
        [flutter close];
        NSLog(@"[MorganLauncher] Flutter 3.22.1 loading screen is unavailable");
    }
}

void MGSetLoadingScreenVisible(bool visible)
{
    MGRunLoadingOnMain(^{
        MGFlutter* screen = flutter;
        if (!screen || screen.visible == visible)
        {
            return;
        }
        screen.visible = visible;
        screen.loadingFinished = NO;
        [screen updateOverlayVisibility];
        [screen send:visible ? @"show" : @"hide" value:nil];
        if (!visible)
        {
            __weak MGFlutter* weakScreen = screen;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                MGFlutter* pending = weakScreen;
                if (pending && pending == flutter && !pending.visible)
                {
                    pending.loadingFinished = YES;
                    [pending updateOverlayVisibility];
                }
            });
        }
    });
}

void MGSetFlutterPauseState(bool paused)
{
    MGRunLoadingOnMain(^{
        MGFlutter* screen = flutter;
        if (screen && screen.paused != paused)
        {
            screen.paused = paused;
            if (screen.chatReady) {
                [screen.chatChannel invokeMethod:@"setPauseState" arguments:@(paused)];
            }
            [screen updateOverlayVisibility];
        }
    });
}

void MGSetFlutterChatVisible(bool visible)
{
    MGRunLoadingOnMain(^{
        MGFlutter* screen = flutter;
        if (screen && screen.chatVisible != visible)
        {
            screen.chatVisible = visible;
            [screen updateOverlayVisibility];
        }
    });
}

void MGSetFlutterSpawnVisible(bool visible)
{
    MGRunLoadingOnMain(^{
        if (spawnVisible != visible)
        {
            spawnVisible = visible;
            [flutter updateSpawnVisibility];
        }
    });
}

void MGCloseFlutterChat()
{
    MGRunLoadingOnMain(^{ [flutter close]; });
}

void MGSetFlutterChatPosition(int position, float radarRight)
{
    const int radarRightPerMille = static_cast<int>(std::ceil(radarRight * 1000.0f));
    MGRunLoadingOnMain(^{
        MGFlutter* screen = flutter;
        if (screen && (!screen.chatLayout || [screen.chatLayout[@"position"] intValue] != position ||
                       [screen.chatLayout[@"radarRight"] intValue] != radarRightPerMille))
        {
            screen.chatLayout = @{@"position": @(position), @"radarRight": @(radarRightPerMille)};
            if (screen.chatReady) {
                [screen.chatChannel invokeMethod:@"ChatPosition" arguments:screen.chatLayout];
            }
        }
    });
}

void MGAddFlutterChatMessage(int type, uint32_t color, uint32_t nickColor, const char* nick, const char* text)
{
    if (!text || !NSThread.isMainThread) return;
    MGFlutter* screen = flutter;
    if (!screen) return;

    NSString* message = [NSString stringWithUTF8String:text];
    if (!message.length) return;
    if (nick && *nick) {
        message = [NSString stringWithFormat:@"%@%@: %@%@", MGChatColor(nickColor), [NSString stringWithUTF8String:nick], MGChatColor(color), message];
    }
    NSDictionary* entry = @{@"type": @(type), @"color": MGChatColor(color), @"message": message};
    if (screen.chatReady) {
        [screen.chatChannel invokeMethod:@"addMessage" arguments:entry];
    } else {
        if (screen.pendingMessages.count == 40) [screen.pendingMessages removeObjectAtIndex:0];
        [screen.pendingMessages addObject:entry];
    }
}

void MGUpdateLoadingScreenPercent(int percent)
{
    MGRunLoadingOnMain(^{
        MGFlutter* screen = flutter;
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
        MGFlutter* screen = flutter;
        if (screen && ![screen.statusText isEqualToString:status])
        {
            screen.statusText = status;
            [screen send:@"updateStatusText" value:status];
        }
    });
}
