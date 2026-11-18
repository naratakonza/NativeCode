#include "../../../../Main.h"
#include "CTouchEvent.h"
#include "../../Multiplayer/CMultiPlayer.h"
#include "../../Multiplayer/Guis/CGui.h"

#include <CoreGraphics/CGGeometry.h>
#include <limits>
#include <mach-o/dyld.h>
#include <objc/message.h>

CTouchEvent::TouchMethod CTouchEvent::m_TouchMethods[] = {
    {"touchesBegan:withEvent:", SA_ADDR(0x32F40), nullptr, nullptr},
    {"touchesMoved:withEvent:", SA_ADDR(0x33128), nullptr, nullptr},
    {"touchesEnded:withEvent:", SA_ADDR(0x332F4), nullptr, nullptr},
    {"touchesCancelled:withEvent:", SA_ADDR(0x334C0), nullptr, nullptr},
};
char CTouchEvent::m_ActiveTouchesKey;
char CTouchEvent::m_GuiTouchKey;
bool CTouchEvent::m_bInstalled;

void CTouchEvent::AddTouch(const void* touch, void* set)
{
    CFSetAddValue(static_cast<CFMutableSetRef>(set), touch);
}

void CTouchEvent::RemoveTouch(const void* touch, void* set)
{
    CFSetRemoveValue(static_cast<CFMutableSetRef>(set), touch);
}

CFMutableSetRef CTouchEvent::GetActiveTouches(id view, bool create)
{
    auto active = reinterpret_cast<CFMutableSetRef>(objc_getAssociatedObject(view, &CTouchEvent::m_ActiveTouchesKey));
    if (!active && create)
    {
        active = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);
        if (active)
        {
            objc_setAssociatedObject(view, &CTouchEvent::m_ActiveTouchesKey, reinterpret_cast<id>(active), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            CFRelease(active);
        }
    }

    return active;
}

void CTouchEvent::CancelActiveTouches(id view, id event, CFMutableSetRef active)
{
    if (!active || !CFSetGetCount(active)) {
        return;
    }

    const CFSetRef touches = CFSetCreateCopy(kCFAllocatorDefault, active);
    if (touches)
    {
        CFSetRemoveAllValues(active);
        CTouchEvent::m_TouchMethods[Cancelled].original(view, CTouchEvent::m_TouchMethods[Cancelled].selector, reinterpret_cast<id>(const_cast<void*>(static_cast<const void*>(touches))), event);
        CFRelease(touches);
    }
}

template <CTouchEvent::TouchPhase phase>
void CTouchEvent::IOS_TouchEvent_hook(id view, SEL selector, id touches, id event)
{
    const auto original = CTouchEvent::m_TouchMethods[phase].original;
    const auto incoming = reinterpret_cast<CFSetRef>(touches);
    const auto active = CTouchEvent::GetActiveTouches(view, phase == Began);

    const bool sampTouch = CMultiPlayer::IsSampActive() && !CGame::IsMenuActive();
    auto* network = sampTouch ? CMultiPlayer::GetActiveNetwork() : nullptr;
    auto* fontRender = CGui::GetFontRender();

    id guiTouch = objc_getAssociatedObject(view, &CTouchEvent::m_GuiTouchKey);
    if constexpr (phase == Began)
    {
        if (!guiTouch && network && fontRender)
        {
            CFSetApplyFunction(incoming, [](const void* touch, void* context)
            {
                auto& primary = *static_cast<id*>(context);
                if (!primary)
                {
                    primary = reinterpret_cast<id>(const_cast<void*>(touch));
                }
            }, &guiTouch);
            if (guiTouch)
            {
                objc_setAssociatedObject(view, &CTouchEvent::m_GuiTouchKey, guiTouch, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
        }
    }

    const bool hasGuiTouch = guiTouch && CFSetContainsValue(incoming, guiTouch);
    if constexpr (phase == Ended || phase == Cancelled)
    {
        if (hasGuiTouch)
        {
            objc_setAssociatedObject(view, &CTouchEvent::m_GuiTouchKey, nullptr, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }

    if (fontRender && hasGuiTouch && (!network || phase == Cancelled))
    {
        fontRender->OnTouchEvent(eTouchType::TOUCH_POP, ImVec2(-1.0f, -1.0f));
    }

    if (sampTouch)
    {
        if (!network || !fontRender)
        {
            if (active)
            {
                CTouchEvent::CancelActiveTouches(view, event, active);
            }
            else if constexpr (phase != Began)
            {
                CTouchEvent::m_TouchMethods[Cancelled].original(view, CTouchEvent::m_TouchMethods[Cancelled].selector, touches, event);
            }

            return;
        }

        constexpr eTouchType iType = phase == Began ? eTouchType::TOUCH_PUSH : phase == Moved ? eTouchType::TOUCH_MOVE : eTouchType::TOUCH_POP;
        ImVec2 position;
        bool hasPosition = false;
        if (hasGuiTouch && phase != Cancelled)
        {
            const CGPoint point = reinterpret_cast<CGPoint (*)(id, SEL, id)>(objc_msgSend)(guiTouch, sel_registerName("locationInView:"), view);
            const id layer = reinterpret_cast<id (*)(id, SEL)>(objc_msgSend)(view, sel_registerName("layer"));
            const float scale = static_cast<float>(reinterpret_cast<CGFloat (*)(id, SEL)>(objc_msgSend)(layer, sel_registerName("contentsScale")));
            position = ImVec2(static_cast<float>(point.x * scale), static_cast<float>(point.y * scale));
            constexpr double intMin = std::numeric_limits<int>::min();
            constexpr double intMax = std::numeric_limits<int>::max();
            hasPosition = scale > 0.0f && position.x >= intMin && position.x <= intMax && position.y >= intMin && position.y <= intMax;
            if (!hasPosition && phase == Ended)
            {
                fontRender->OnTouchEvent(eTouchType::TOUCH_POP, ImVec2(-1.0f, -1.0f));
            }
        }

        if (CGui::GetDialog() && CGui::GetDialog()->GetActiveDialog())
        {
            if (hasPosition)
            {
                if (CGui::GetSpawnScreen() && CGui::GetSpawnScreen()->GetActiveSpawnScreen())
                {
                    if (!fontRender->OnTouchEvent(iType, position))
                    {
                        return;
                    }
                }
                else
                {
                    fontRender->OnTouchEvent(eTouchType::TOUCH_POP, ImVec2(-1.0f, -1.0f));
                }
            }

            CTouchEvent::CancelActiveTouches(view, event, active);
            return;
        }

        // if (CGui::GetScoreBoard()->GetActiveScore() || CGui::GetMainMenuScreens()->GetActiveMainMenuScreens() || CGui::GetVoiceMenu()->GetActiveVoiceMenu() /*|| CGui::GetClientMenu()->GetActiveClientMenu() || CGui::GetChatMenu()->GetActiveChatMenu()*/)
        // {
        //     CTouchEvent::CancelActiveTouches(view, event, active);
        //     return;
        // }

        if (auto* pools = network->GetNetGamePools())
        {
            if (auto* textDrawPool = pools->GetTextDrawPool())
            {
                const bool consumed = hasPosition ? !textDrawPool->OnTouchEvent(iType, CFSetGetCount(incoming) > 1, static_cast<int>(position.x), static_cast<int>(position.y)) : textDrawPool->GetState();
                if (consumed)
                {
                    if (hasPosition && !fontRender->OnTouchEvent(iType, position))
                    {
                        return;
                    }

                    CTouchEvent::CancelActiveTouches(view, event, active);
                    return;
                }
            }
        }

        if (hasPosition && !fontRender->OnTouchEvent(iType, position))
        {
            CTouchEvent::CancelActiveTouches(view, event, active);
            return;
        }
    }

    if (!active)
    {
        original(view, selector, touches, event);
        return;
    }

    if constexpr (phase == Began)
    {
        CFSetApplyFunction(incoming, AddTouch, active);
        original(view, selector, touches, event);
        return;
    }

    TrackedTouches tracked{active, 0, nullptr};
    CFSetApplyFunction(incoming, [](const void* touch, void* context)
    {
        auto& state = *static_cast<TrackedTouches*>(context);
        state.count += CFSetContainsValue(state.active, touch);
    }, &tracked);
    if (!tracked.count) {
        return;
    }

    CFSetRef forwarded = incoming;
    if (tracked.count != CFSetGetCount(incoming))
    {
        tracked.subset = CFSetCreateMutable(kCFAllocatorDefault, tracked.count, &kCFTypeSetCallBacks);
        if (!tracked.subset) {
            return;
        }
        CFSetApplyFunction(incoming, [](const void* touch, void* context)
        {
            auto& state = *static_cast<TrackedTouches*>(context);
            if (CFSetContainsValue(state.active, touch))
            {
                CFSetAddValue(state.subset, touch);
            }
        }, &tracked);
        forwarded = tracked.subset;
    }

    if constexpr (phase == Ended || phase == Cancelled)
    {
        CFSetApplyFunction(forwarded, RemoveTouch, active);
    }

    original(view, selector, reinterpret_cast<id>(const_cast<void*>(static_cast<const void*>(forwarded))), event);

    if (tracked.subset)
    {
        CFRelease(tracked.subset);
    }
}

void CTouchEvent::Initialise()
{
    if (CTouchEvent::m_bInstalled || !pthread_main_np()) {
        return;
    }

    CNecessary::Log(OBFUSCATE("CTouchEvent Initialise"));

    const Class view = reinterpret_cast<Class>(objc_getClass("EAGLView"));
    if (!view) {
        return;
    }

    const auto slide = _dyld_get_image_vmaddr_slide(0);
    Method methods[4]{};
    const TouchCallback hooks[] = {
        IOS_TouchEvent_hook<Began>,
        IOS_TouchEvent_hook<Moved>,
        IOS_TouchEvent_hook<Ended>,
        IOS_TouchEvent_hook<Cancelled>,
    };

    for (std::size_t i = 0; i < ARRAY_SIZE(CTouchEvent::m_TouchMethods); ++i)
    {
        auto& touch = CTouchEvent::m_TouchMethods[i];
        touch.selector = sel_registerName(touch.name);
        methods[i] = class_getInstanceMethod(view, touch.selector);
        if (!methods[i] || std::strcmp(method_getTypeEncoding(methods[i]), "v32@0:8@16@24") != 0 || reinterpret_cast<std::uintptr_t>(method_getImplementation(methods[i])) != touch.address + slide || !CMemory::HasMappedBytes(touch.address, "\xED\x33\xB8\x6D\xEB\x2B\x01\x6D", 8, "__text"))
        {
            CNecessary::Log(OBFUSCATE("CTouchEvent: GTA iOS touch callback mismatch"));
            return;
        }

        touch.original = reinterpret_cast<TouchCallback>(method_getImplementation(methods[i]));
    }

    for (std::size_t i = 0; i < ARRAY_SIZE(CTouchEvent::m_TouchMethods); ++i)
    {
        method_setImplementation(methods[i], reinterpret_cast<IMP>(hooks[i]));
    }

    CTouchEvent::m_bInstalled = true;
}