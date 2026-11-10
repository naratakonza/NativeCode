#pragma once

#ifdef __OBJC__
@class UIViewController;

void MGPrepareLoadingScreen(UIViewController* parent);
#endif

void MGSetLoadingScreenVisible(bool visible);
void MGUpdateLoadingScreenPercent(int percent);
void MGUpdateLoadingScreenList(const char* text);