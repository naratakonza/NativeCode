#pragma once

#include <cstdint>

#ifdef __OBJC__
@class UIViewController;

void MGPrepareLoadingScreen(UIViewController* parent);
#endif

void MGSetLoadingScreenVisible(bool visible);
void MGUpdateLoadingScreenPercent(int percent);
void MGUpdateLoadingScreenList(const char* text);
void MGSetFlutterPauseState(bool paused);
void MGSetFlutterChatVisible(bool visible);
void MGSetFlutterChatPosition(int position, float radarRight);
void MGCloseFlutterChat();
void MGAddFlutterChatMessage(int type, uint32_t color, uint32_t nickColor, const char* nick, const char* text);
