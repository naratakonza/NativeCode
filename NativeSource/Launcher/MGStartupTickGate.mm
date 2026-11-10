#import "MGStartupTickGate.h"
#import "MGStartupTickState.h"

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#import <objc/runtime.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>

#include "../source/game_morgan/CMainMenuScreen.h"
#include "../source/utils/FileIO/FileOpenDiagnostics.h"
#include "../source/utils/Memory/CMemory.h"
#include "../source/utils/Necessary/CNecessary.h"
#include "../source/utils/Storage/CStorage.h"

@protocol MGFlutterStartupSemantics <NSObject>
- (id<MGFlutterStartupSemantics>)engine;
- (void)ensureSemanticsEnabled;
@end

namespace {
Method gatedTick = nullptr;
morgan::launcher::StartupTickState<IMP> tickState;

constexpr unsigned char gtaUUID[] = {0x0e, 0x42, 0xe8, 0xf2, 0x69, 0x92, 0x3e, 0xe9,
                                     0xa7, 0x45, 0xb3, 0xfd, 0x65, 0x69, 0xa9, 0xfd};
constexpr unsigned char rockstarUUID[] = {0x5f, 0xa6, 0x58, 0x7e, 0xf7, 0x2d, 0x34, 0xf2,
                                          0xac, 0x8e, 0x80, 0x3e, 0x77, 0x9b, 0x3b, 0xd6};

bool MGMatchesImage(const void *address, std::uint32_t filetype, const unsigned char (&expected)[16]) {
    Dl_info info{};
    if (!address || !dladdr(address, &info) || !info.dli_fbase) return false;
    const auto *header = static_cast<const mach_header_64*>(info.dli_fbase);
    if (header->magic != MH_MAGIC_64 || header->filetype != filetype ||
        header->cputype != CPU_TYPE_ARM64 || header->sizeofcmds > 1024 * 1024) return false;
    const auto *cursor = reinterpret_cast<const unsigned char*>(header + 1);
    std::size_t remaining = header->sizeofcmds;
    for (std::uint32_t i = 0; i < header->ncmds && remaining >= sizeof(load_command); ++i) {
        load_command command{};
        std::memcpy(&command, cursor, sizeof(command));
        if (command.cmdsize < sizeof(command) || command.cmdsize > remaining) return false;
        if (command.cmd == LC_UUID && command.cmdsize == sizeof(uuid_command)) {
            return std::memcmp(cursor + sizeof(load_command), expected, sizeof(expected)) == 0;
        }
        cursor += command.cmdsize;
        remaining -= command.cmdsize;
    }
    return false;
}

using PresentModal = void (*)(UIViewController *, SEL, UIViewController *, BOOL, dispatch_block_t);
using DismissModal = void (*)(UIViewController *, SEL, BOOL, dispatch_block_t);

PresentModal originalPresent = nullptr;
DismissModal originalDismiss = nullptr;
Class accountHostClass = Nil;
Class accountScreenClass = Nil;
Ivar accountScreenField = nullptr;
Ivar accountModeField = nullptr;
__weak UIViewController *startupAccountHost;
__weak UIViewController *startupAccountPresenter;
bool startupAccountPresented = false;

template <typename Value>
Value MGReadSDKField(id object, Ivar field) {
    const auto offset = static_cast<std::size_t>(ivar_getOffset(field));
    const auto size = class_getInstanceSize(object_getClass(object));
    Value result{};
    if (offset <= size && sizeof(result) <= size - offset) {
        std::memcpy(&result, static_cast<const char*>((__bridge const void*)object) + offset, sizeof(result));
    }
    return result;
}

bool MGIsStartupAccountPrompt(UIViewController *host) {
    if (!host || object_getClass(host) != accountHostClass) return false;
    // On this SDK UUID, FlutterScreenHost strongly owns its Swift screen object.
    id screen = (__bridge id)MGReadSDKField<const void*>(host, accountScreenField);
    // RockstarMobile passes mode 1 when creating the gameload account options.
    return screen && object_getClass(screen) == accountScreenClass &&
        MGReadSDKField<std::uint8_t>(screen, accountModeField) == 1;
}

UIAccessibilityElement *MGFindStartupSkipButton(UIView *root)
{
    if (!root) return nil;

    constexpr NSUInteger nodeLimit = 256;
    NSMutableArray *nodes = [NSMutableArray arrayWithObject:root];
    NSHashTable *seen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    [seen addObject:root];
    auto addNode = [&](id node) {
        if (node && ![seen containsObject:node])
        {
            [seen addObject:node];
            [nodes addObject:node];
        }
    };

    UIAccessibilityElement *button = nil;
    for (NSUInteger i = 0; i < nodes.count; ++i)
    {
        if (nodes.count > nodeLimit) return nil;
        id node = nodes[i];
        if ([node isKindOfClass:UIView.class])
        {
            UIView *view = node;
            if (view.hidden || view.alpha <= 0.01 || view.accessibilityElementsHidden) continue;
            for (UIView *child in view.subviews) addNode(child);
        }

        if ([node isKindOfClass:UIAccessibilityElement.class])
        {
            UIAccessibilityElement *element = node;
            if (element.isAccessibilityElement && !(element.accessibilityTraits & UIAccessibilityTraitNotEnabled) &&
                [element.accessibilityLabel isEqualToString:@"Skip Sign in"])
            {
                if (button) return nil;
                button = element;
            }
        }

        if ([node respondsToSelector:@selector(accessibilityElements)])
        {
            for (id child in [node accessibilityElements])
            {
                addNode(child);
                if (nodes.count > nodeLimit) return nil;
            }
        }
        if ([node respondsToSelector:@selector(accessibilityElementCount)] &&
            [node respondsToSelector:@selector(accessibilityElementAtIndex:)])
        {
            const NSInteger count = [node accessibilityElementCount];
            if (count == NSNotFound) continue;
            if (count > static_cast<NSInteger>(nodeLimit)) return nil;
            for (NSInteger index = 0; index < count; ++index) addNode([node accessibilityElementAtIndex:index]);
        }
    }
    return nodes.count <= nodeLimit ? button : nil;
}

void MGTryStartupSkip(UIViewController *presenter, UIViewController *host, UIView *cover, unsigned remaining)
{
    if (host != startupAccountHost || presenter.presentedViewController != host ||
        host.presentedViewController || !MGIsStartupAccountPrompt(host))
    {
        [cover removeFromSuperview];
        return;
    }

    UIAccessibilityElement *button = host.viewIfLoaded.window ? MGFindStartupSkipButton(host.viewIfLoaded) : nil;
    if (button && [button accessibilityActivate])
    {
        CNecessary::Log(OBFUSCATE("Rockstar Skip Sign in action activated"));
        // Let the SDK dismiss its own prompt. Reveal it if it stays open.
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            [cover removeFromSuperview];
        });
        return;
    }

    if (!remaining)
    {
        [cover removeFromSuperview];
        return;
    }

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 8), dispatch_get_main_queue(), ^{
        MGTryStartupSkip(presenter, host, cover, remaining - 1);
    });
}

void MGPresentStartupAccount(UIViewController *presenter, SEL selector, UIViewController *host,
                             BOOL animated, dispatch_block_t completion) {
    if (NSThread.isMainThread && !startupAccountPresented && !presenter.presentedViewController &&
        MGMatchesImage(__builtin_extract_return_addr(__builtin_return_address(0)), MH_DYLIB, rockstarUUID) &&
        MGIsStartupAccountPrompt(host)) {
        startupAccountPresented = true;
        startupAccountHost = host;
        startupAccountPresenter = presenter;
        animated = NO;

        id<MGFlutterStartupSemantics> flutterHost = (id<MGFlutterStartupSemantics>)host;
        id<MGFlutterStartupSemantics> engine = [flutterHost respondsToSelector:@selector(engine)] ? [flutterHost engine] : nil;
        if ([engine respondsToSelector:@selector(ensureSemanticsEnabled)])
        {
            // Keep the real Flutter screen active under the cover so it can expose its own Skip action.
            UIView *cover = [[UIView alloc] initWithFrame:host.view.bounds];
            cover.backgroundColor = UIColor.blackColor;
            cover.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            cover.accessibilityElementsHidden = YES;
            [host.view addSubview:cover];

            dispatch_block_t originalCompletion = completion;
            completion = ^{
                if (originalCompletion) originalCompletion();
                [engine ensureSemanticsEnabled];
                MGTryStartupSkip(presenter, host, cover, 24);
            };
        }
    }
    originalPresent(presenter, selector, host, animated, completion);
}

void MGDismissStartupAccount(UIViewController *presenter, SEL selector, BOOL animated, dispatch_block_t completion) {
    if (NSThread.isMainThread && presenter == startupAccountPresenter &&
        MGMatchesImage(__builtin_extract_return_addr(__builtin_return_address(0)), MH_DYLIB, rockstarUUID)) {
        UIViewController *host = startupAccountHost;
        if (host && presenter.presentedViewController == host && !host.presentedViewController) {
            startupAccountHost = nil;
            startupAccountPresenter = nil;
            animated = NO;
        }
    }
    // Rockstar decides when to dismiss. UIKit receives the original SDK completion.
    originalDismiss(presenter, selector, animated, completion);
}

void MGInstallStartupAccountTransitions(Class gtaController) {
    const SEL presentSelector = @selector(presentViewController:animated:completion:);
    const SEL dismissSelector = @selector(dismissViewControllerAnimated:completion:);
    const Method present = class_getInstanceMethod(UIViewController.class, presentSelector);
    const Method dismiss = class_getInstanceMethod(UIViewController.class, dismissSelector);
    // Add overrides only for the verified GTA presenter. Leave custom implementations alone.
    if (!present || !dismiss || class_getInstanceMethod(gtaController, presentSelector) != present ||
        class_getInstanceMethod(gtaController, dismissSelector) != dismiss) return;

    const Class host = objc_lookUpClass("_TtC14RockstarMobile17FlutterScreenHost");
    const Class screen = objc_lookUpClass("_TtC14RockstarMobile28FlutterSocialClubLoginScreen");
    const Method sdkMethod = host ? class_getInstanceMethod(host, @selector(shouldAutorotate)) : nullptr;
    if (!screen || !sdkMethod ||
        !MGMatchesImage(reinterpret_cast<const void*>(method_getImplementation(sdkMethod)), MH_DYLIB, rockstarUUID)) return;

    accountScreenField = class_getInstanceVariable(host, "screen");
    accountModeField = class_getInstanceVariable(screen, "mode");
    if (!accountScreenField || !accountModeField) return;

    accountHostClass = host;
    accountScreenClass = screen;
    originalPresent = reinterpret_cast<PresentModal>(method_getImplementation(present));
    originalDismiss = reinterpret_cast<DismissModal>(method_getImplementation(dismiss));
    if (!class_addMethod(gtaController, dismissSelector, reinterpret_cast<IMP>(&MGDismissStartupAccount),
                         method_getTypeEncoding(dismiss))) return;
    class_addMethod(gtaController, presentSelector, reinterpret_cast<IMP>(&MGPresentStartupAccount),
                    method_getTypeEncoding(present));
}

void MGDeferredGtaTick(id, SEL) {
    if (NSThread.isMainThread) GameStartupTrace().Note(StartupPhase::FirstTickDeferred);
}

Method menuTick = nullptr;
IMP originalMenuTick = nullptr;
using SetSplashText = void (*)(id, SEL, NSString *);
SetSplashText originalSplashText = nullptr;
struct GTAMenuStack
{
    std::uint32_t capacity;
    std::uint32_t count;
    const std::uintptr_t *screens;
};
static_assert(offsetof(GTAMenuStack, screens) == 8);

const std::uint32_t *startupState = nullptr;
const GTAMenuStack *gameMenuStack = nullptr;
std::uintptr_t gameMainMenuVtable = 0;
float *splashNoticeTimer = nullptr;
bool menuUpdateStarted = false;
constexpr std::uint32_t splashNoticeState = 1;
constexpr std::uint32_t hideSplashState = 2;
constexpr std::uint32_t mainMenuState = 8;
constexpr std::uint32_t loadedMenuState = 10;
constexpr float splashNoticeDuration = 3.0f;

void MGSkipTimedSplashText(id controller, SEL selector, NSString *text)
{
    if (NSThread.isMainThread && (*startupState == splashNoticeState || *startupState == hideSplashState)) return;
    originalSplashText(controller, selector, text);
}

void MGInstallSampStartup(Class controller)
{
    if (startupState) return;

    // Verify the dispatcher state and its State 7 call that queues MainMenuScreen.
    if (!CMemory::HasMappedInstruction(SA_ADDR(0x43ABA4), "\x74\x27\x00\xD0") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43ABA8), "\x94\x72\x2B\x91") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43ABAC), "\x88\x02\x40\xB9") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43ACC4), "\x44\x8A\xF6\x97")) return;

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    startupState = reinterpret_cast<const std::uint32_t*>(SA_ADDR(0x928ADC) + slide);

    // Verify the active stack reader and the native MainMenuScreen vtable.
    if (CMemory::HasMappedInstruction(SA_ADDR(0x1DB9B0), "\xA8\x2D\x00\xB0") &&
        CMemory::HasMappedInstruction(SA_ADDR(0x1DB9B4), "\x08\x31\x38\x91") &&
        CMemory::HasMappedInstruction(SA_ADDR(0x1DB9B8), "\x09\x41\x40\xF8") &&
        CMemory::HasMappedInstruction(SA_ADDR(0x1DD60C), "\x08\x41\x00\x91") &&
        CMemory::HasMappedInstruction(SA_ADDR(0x1DE450), "\x08\x54\x40\x39"))
    {
        const auto vtable = SA_ADDR(0x4A0268) + slide;
        const auto *slots = reinterpret_cast<const std::uintptr_t*>(vtable);
        if (slots[2] == SA_ADDR(0x1DE438) + slide)
        {
            gameMenuStack = reinterpret_cast<const GTAMenuStack*>(SA_ADDR(0x790E08) + slide);
            gameMainMenuVtable = vtable;
        }
    }

    const auto method = class_getInstanceMethod(controller, sel_registerName("setSplashText:"));
    const char *types = method ? method_getTypeEncoding(method) : nullptr;
    const auto original = method ? method_getImplementation(method) : nullptr;
    if (!types || std::strcmp(types, "v24@0:8@16") != 0 || reinterpret_cast<std::uintptr_t>(original) != SA_ADDR(0x3CFE4) + slide) return;

    // Advance the timer through GTA's own state-1 branch. The timer is in writable __DATA.
    if (!CMemory::HasMappedInstruction(SA_ADDR(0x43ACDC), "\xF5\x11\x00\xB0") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43ACE0), "\xA0\xC2\x41\xBD") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43ACE8), "\xA0\xC2\x01\xBD") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43B268), "\x20\x05\x40\xBD") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43B26C), "\x01\x10\x21\x1E") ||
        !CMemory::HasMappedInstruction(SA_ADDR(0x43B274), "\x6B\xF5\xFF\x54")) return;

    splashNoticeTimer = reinterpret_cast<float*>(SA_ADDR(0x6771C0) + slide);
    originalSplashText = reinterpret_cast<SetSplashText>(original);
    method_setImplementation(method, reinterpret_cast<IMP>(&MGSkipTimedSplashText));
}

bool MGIsMainMenuActive()
{
    const auto count = gameMenuStack->count;
    if (!count || count > gameMenuStack->capacity || !gameMenuStack->screens) return false;

    const auto screen = gameMenuStack->screens[count - 1];
    if (!screen || screen % alignof(std::uintptr_t)) return false;
    std::uintptr_t vtable{};
    std::memcpy(&vtable, reinterpret_cast<const void*>(screen), sizeof(vtable));
    return vtable == gameMainMenuVtable;
}

void MGObserveMainMenuTick(id controller, SEL selector)
{
    const auto original = originalMenuTick;
    const bool mainThread = NSThread.isMainThread;
    if (mainThread && splashNoticeTimer && *startupState == splashNoticeState) *splashNoticeTimer = splashNoticeDuration;

    reinterpret_cast<void (*)(id, SEL)>(original)(controller, selector);
    if (!mainThread || menuUpdateStarted) return;
    if (gameMenuStack)
    {
        const auto state = *startupState;
        if ((state != mainMenuState && state != loadedMenuState) || [(UIViewController *)controller presentedViewController] || !MGIsMainMenuActive()) return;

        // State 8 waits for the start request. State 10 can follow it.
        CNecessary::Log(OBFUSCATE("GTA MainMenuScreen active (startup state %u)"), state);
        menuUpdateStarted = true;
        menuUpdateStarted = CMainMenuScreen::Update();
        if (!menuUpdateStarted) return;
    }

    if (method_getImplementation(menuTick) == reinterpret_cast<IMP>(&MGObserveMainMenuTick)) {
        method_setImplementation(menuTick, original);
    }
}

void MGObserveFirstGtaTick(id controller, SEL selector) {
    if (!NSThread.isMainThread || !gatedTick) return;
    const auto observer = reinterpret_cast<IMP>(&MGObserveFirstGtaTick);
    if (method_getImplementation(gatedTick) != observer) return;
    const auto original = tickState.TakeOriginal(observer);
    if (!original) return;
    originalMenuTick = original;
    menuTick = gatedTick;
    method_setImplementation(gatedTick, reinterpret_cast<IMP>(&MGObserveMainMenuTick));
    gatedTick = nullptr;
    GameStartupTrace().Note(StartupPhase::FirstTickStarted);
    Class gtaController = objc_lookUpClass("IOSViewController");
    if (gtaController && object_getClass(controller) == gtaController) {
        MGInstallStartupAccountTransitions(gtaController);
        MGInstallSampStartup(gtaController);
    }
    reinterpret_cast<void (*)(id, SEL)>(original)(controller, selector);
    GameStartupTrace().Note(StartupPhase::FirstTickCompleted);
}
using StartAnimation = void (*)(id, SEL);
StartAnimation originalStartAnimation = nullptr;
int preferredGtaFramerate = 0;

void MGApplyGtaFramerate()
{
    // IOSViewController::startAnimation owns this CADisplayLink and replaces it on resume.
    const auto address = SA_ADDR(0x4EB3A0) + _dyld_get_image_vmaddr_slide(0);
    const void* value = nullptr;
    std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
    CADisplayLink* link = (__bridge CADisplayLink*)value;
    if ([link isKindOfClass:CADisplayLink.class])
    {
        const auto fps = static_cast<float>(preferredGtaFramerate);
        link.preferredFrameRateRange = CAFrameRateRangeMake(30.0f, fps, fps);
    }
}

void MGStartGtaAnimation(id controller, SEL selector)
{
    originalStartAnimation(controller, selector);
    MGApplyGtaFramerate();
}
} // namespace

bool MGSetGtaFramerate(int fps)
{
    if (!NSThread.isMainThread) return false;

    if (!originalStartAnimation)
    {
        const Class controller = objc_lookUpClass("IOSViewController");
        const Method method = controller ? class_getInstanceMethod(controller, sel_registerName("startAnimation")) : nullptr;
        const char* types = method ? method_getTypeEncoding(method) : nullptr;
        const IMP original = method ? method_getImplementation(method) : nullptr;
        const auto address = SA_ADDR(0x3C9D4) + _dyld_get_image_vmaddr_slide(0);
        if (!types || std::strcmp(types, "v16@0:8") != 0 || reinterpret_cast<std::uintptr_t>(original) != address ||
            !CMemory::HasMappedInstruction(SA_ADDR(0x3CA54), "\xA0\xD2\x01\xF9") ||
            !CMemory::HasMappedInstruction(SA_ADDR(0x3CA68), "\xA0\xD2\x41\xF9") ||
            !CMemory::HasMappedInstruction(SA_ADDR(0x3CA70), "\x4C\x11\x10\x94")) return false;

        originalStartAnimation = reinterpret_cast<StartAnimation>(original);
        method_setImplementation(method, reinterpret_cast<IMP>(&MGStartGtaAnimation));
    }

    preferredGtaFramerate = fps;
    MGApplyGtaFramerate();
    return true;
}

void MGObserveBundledSampMenu() {
    if (!NSThread.isMainThread || menuTick) return;

    const Class controller = objc_lookUpClass("IOSViewController");
    const Method tick = controller ? class_getInstanceMethod(controller, sel_registerName("tick")) : nullptr;
    const char* types = tick ? method_getTypeEncoding(tick) : nullptr;
    const IMP original = tick ? method_getImplementation(tick) : nullptr;
    if (!types || std::strcmp(types, "v16@0:8") != 0 || !MGMatchesImage(reinterpret_cast<const void*>(original), MH_EXECUTE, gtaUUID)) return;

    originalMenuTick = original;
    menuTick = tick;
    MGInstallSampStartup(controller);
    method_setImplementation(menuTick, reinterpret_cast<IMP>(&MGObserveMainMenuTick));
}

bool MGOwnsDeferredTick() {
    return NSThread.isMainThread && gatedTick &&
           tickState.OwnsDeferred(method_getImplementation(gatedTick));
}

void MGInstallAssetFreeTickGate() {
    if (!morgan::fileio::CStorage::IsAssetFreeBundle() || gatedTick) return;
    if (!NSThread.isMainThread) {
        GameStartupTrace().Note(StartupPhase::TickGateRejected);
        NSLog(@"[MorganLauncher] Startup tick gate requires the main thread");
        return;
    }
    Class controller = objc_lookUpClass("IOSViewController");
    Method tick = controller ? class_getInstanceMethod(controller, sel_registerName("tick")) : nullptr;
    const char* types = tick ? method_getTypeEncoding(tick) : nullptr;
    const auto original = tick ? method_getImplementation(tick) : nullptr;
    const auto deferred = reinterpret_cast<IMP>(&MGDeferredGtaTick);
    if (!types || std::strcmp(types, "v16@0:8") != 0 || !MGMatchesImage(reinterpret_cast<const void*>(original), MH_EXECUTE, gtaUUID) ||
        !tickState.Install(original, deferred)) {
        GameStartupTrace().Note(StartupPhase::TickGateRejected);
        NSLog(@"[MorganLauncher] Startup tick gate rejected the GTA class, signature, or original IMP");
        return;
    }
    method_setImplementation(tick, deferred);
    gatedTick = tick;
    GameStartupTrace().Note(StartupPhase::TickGateInstalled);
}

bool MGReleaseDeferredTick() {
    if (!NSThread.isMainThread || !gatedTick) return false;
    const auto observer = reinterpret_cast<IMP>(&MGObserveFirstGtaTick);
    if (!tickState.Observe(method_getImplementation(gatedTick), observer)) return false;
    method_setImplementation(gatedTick, observer);
    return true;
}
