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
void MGSetFlutterSpawnVisible(bool visible);
void MGShowFlutterDialog(uint16_t id, uint8_t style, const char* title, const char* content, const char* button1, const char* button2);
void MGHideFlutterDialog();
void MGSetFlutterChatPosition(int position, float radarRight);
void MGCloseFlutterChat();
void MGAddFlutterChatMessage(int type, uint32_t color, uint32_t nickColor, const char* nick, const char* text);

bool MGUpdateFlutterKeysBar(bool visible, bool inVehicle, bool menuActive, bool f4Enabled, int radarLeft, int radarBottom);

bool MGGetFlutterKeysBarBottom(float& bottom);

void MGSetFlutterScoreBoardVisible(bool visible);
void MGSetFlutterScoreBoardInfo(const char* serverName, int online);
void MGSetFlutterScoreBoardLocalPlayer(int id);
void MGUpdateFlutterScoreBoardPlayer(int id, const char* name, int score, int ping, uint32_t color);
