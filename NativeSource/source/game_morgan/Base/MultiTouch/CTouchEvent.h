#pragma once

#include <cstdint>
#include <CoreFoundation/CFSet.h>
#include <objc/runtime.h>

class CTouchEvent
{
public:
    static void Initialise();

private:
    enum TouchPhase { Began, Moved, Ended, Cancelled };
    using TouchCallback = void (*)(id, SEL, id, id);

    struct TouchMethod
    {
        const char* name;
        std::uint64_t address;
        SEL selector;
        TouchCallback original;
    };
    
    struct TrackedTouches
    {
        CFSetRef active;
        CFIndex count;
        CFMutableSetRef subset;
    };

    static void AddTouch(const void* touch, void* set);
    static void RemoveTouch(const void* touch, void* set);
    static CFMutableSetRef GetActiveTouches(id view, bool create);
    static void CancelActiveTouches(id view, id event, CFMutableSetRef active);

    template <TouchPhase phase>
    static void IOS_TouchEvent_hook(id view, SEL selector, id touches, id event);

    static TouchMethod m_TouchMethods[];
    static char m_ActiveTouchesKey;
    static bool m_bInstalled;
};