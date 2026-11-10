#import "MGFlutterLoadingScreen.h"

#import <UIKit/UIKit.h>
#import <dispatch/dispatch.h>

#include "../Main.h"
#include "../source/game_morgan/Multiplayer/Guis/CChat.h"

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
@end

@implementation MGFlutterOverlayView
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent*)event
{
    return [super pointInside:point withEvent:event] && CGRectContainsPoint(self.chatArea, point);
}
@end

@interface MGFlutterLoadingScreen : NSObject
@property(nonatomic, strong) id<MGLoadingEngine> engine;
@property(nonatomic, strong) UIViewController<MGLoadingController>* controller;
@property(nonatomic, strong) id<MGLoadingChannel> channel;
@property(nonatomic, strong) id<MGLoadingChannel> chatChannel;
@property(nonatomic, strong) MGFlutterOverlayView* overlay;
@property(nonatomic, strong) NSMutableArray<NSDictionary*>* pendingMessages;
@property(nonatomic, copy) NSString* statusText;
@property(nonatomic, strong) NSNumber* percent;
@property(nonatomic) BOOL ready;
@property(nonatomic) BOOL visible;
@property(nonatomic) BOOL chatReady;
@property(nonatomic) BOOL chatVisible;
@property(nonatomic) BOOL loadingFinished;
@property(nonatomic, strong) NSNumber* chatPosition;
- (BOOL)attachTo:(UIViewController*)parent;
- (void)close;
- (void)send:(NSString*)method value:(id)value;
- (void)updateChatVisibility;
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

static NSStringEncoding MGChatEncoding()
{
    return CFStringConvertEncodingToNSStringEncoding(CNecessary::GetLocalizedEncoding());
}

static NSString* MGChatColor(uint32_t rgba)
{
    return [NSString stringWithFormat:@"{%06X}", static_cast<unsigned>(rgba >> 8)];
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
    self.chatChannel = [channelClass methodChannelWithName:@"flutter_chat" binaryMessenger:[self.engine binaryMessenger]];
    self.pendingMessages = [NSMutableArray array];
    self.controller = [[(id)controllerClass alloc] initWithEngine:self.engine nibName:nil bundle:nil];
    if (!self.channel || !self.chatChannel || !self.controller)
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
                    screen.loadingFinished = YES;
                    [screen updateChatVisibility];
                }
            });
        }
        result(nil);
    }];
    [self.chatChannel setMethodCallHandler:^(id<MGLoadingCall> call, MGLoadingResult result) {
        MGFlutterLoadingScreen* screen = weakSelf;
        if (!screen || screen != loadingScreen) {
            result(@NO);
            return;
        }

        if ([call.method isEqualToString:@"ready"])
        {
            screen.chatReady = YES;
            for (NSDictionary* message in screen.pendingMessages) {
                [screen.chatChannel invokeMethod:@"addMessage" arguments:message];
            }
            [screen.pendingMessages removeAllObjects];
            if (screen.chatPosition) {
                [screen.chatChannel invokeMethod:@"ChatPosition" arguments:screen.chatPosition];
            }
            [screen updateChatVisibility];
            result(@YES);
            return;
        }

        NSDictionary* args = [call.arguments isKindOfClass:NSDictionary.class] ? call.arguments : nil;
        if ([call.method isEqualToString:@"sendText"] && screen.chatVisible && screen.loadingFinished)
        {
            NSString* text = [args[@"text"] isKindOfClass:NSString.class] ? args[@"text"] : nil;
            NSData* encoded = [text dataUsingEncoding:MGChatEncoding()] ?: [text dataUsingEncoding:NSUTF8StringEncoding];
            const bool queued = encoded.length && CChat::QueueInput(std::string(static_cast<const char*>(encoded.bytes), encoded.length));
            result(@(queued));
            return;
        }
        if ([call.method isEqualToString:@"hitRegion"])
        {
            CGRect area = CGRectZero;
            if (screen.chatVisible && screen.loadingFinished && [args[@"x"] isKindOfClass:NSNumber.class] && [args[@"y"] isKindOfClass:NSNumber.class] &&
                [args[@"width"] isKindOfClass:NSNumber.class] && [args[@"height"] isKindOfClass:NSNumber.class])
            {
                const auto x = [args[@"x"] doubleValue];
                const auto y = [args[@"y"] doubleValue];
                const auto width = [args[@"width"] doubleValue];
                const auto height = [args[@"height"] doubleValue];
                if (std::isfinite(x) && std::isfinite(y) && std::isfinite(width) && std::isfinite(height) && width > 0 && height > 0) {
                    area = CGRectIntersection(CGRectMake(x, y, width, height), screen.overlay.bounds);
                }
            }
            screen.overlay.chatArea = area;
            result(@YES);
            return;
        }
        result(@NO);
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
            MGSetLoadingScreenVisible(false);
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

- (void)updateChatVisibility
{
    const BOOL show = self.chatVisible && self.loadingFinished && !self.visible;
    self.controller.viewIfLoaded.accessibilityElementsHidden = !show;
    if (!show) {
        self.overlay.chatArea = CGRectZero;
    }
    if (self.chatReady) {
        [self.chatChannel invokeMethod:show ? @"show" : @"hide" arguments:nil];
    }
}

- (void)close
{
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
        screen.loadingFinished = NO;
        [screen updateChatVisibility];
        [screen send:visible ? @"show" : @"hide" value:nil];
        if (!visible)
        {
            __weak MGFlutterLoadingScreen* weakScreen = screen;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                MGFlutterLoadingScreen* pending = weakScreen;
                if (pending && pending == loadingScreen && !pending.visible)
                {
                    pending.loadingFinished = YES;
                    [pending updateChatVisibility];
                }
            });
        }
    });
}

void MGSetFlutterChatVisible(bool visible)
{
    MGRunLoadingOnMain(^{
        MGFlutterLoadingScreen* screen = loadingScreen;
        if (screen && screen.chatVisible != visible)
        {
            screen.chatVisible = visible;
            [screen updateChatVisibility];
        }
    });
}

void MGCloseFlutterChat()
{
    MGRunLoadingOnMain(^{ [loadingScreen close]; });
}

void MGSetFlutterChatPosition(int position)
{
    MGRunLoadingOnMain(^{
        MGFlutterLoadingScreen* screen = loadingScreen;
        if (screen && ![screen.chatPosition isEqualToNumber:@(position)])
        {
            screen.chatPosition = @(position);
            if (screen.chatReady) {
                [screen.chatChannel invokeMethod:@"ChatPosition" arguments:screen.chatPosition];
            }
        }
    });
}

void MGAddFlutterChatMessage(int type, uint32_t color, uint32_t nickColor, const char* nick, const char* text)
{
    if (!text || !NSThread.isMainThread) return;
    MGFlutterLoadingScreen* screen = loadingScreen;
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
