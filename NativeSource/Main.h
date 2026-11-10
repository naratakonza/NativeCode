#pragma once

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <strings.h>

#include "source/game_morgan/Header/Common.h"
#include "source/game_morgan/RenderWare/RenderWare.h"
#include "source/game_morgan/CClock.h"
#include "source/game_morgan/CGame.h"
#include "source/game_morgan/CSetting.h"
#include "source/game_morgan/Scripts/CScripting.h"
#include "source/utils/Exceptions/CExceptions.h"
#include "source/utils/Necessary/CNecessary.h"
#include "source/utils/Memory/CMemory.h"

#define SAMP_VERSION	OBFUSCATE("0.3.7 R5")
#define PORT_VERSION	OBFUSCATE("0.69")

#ifndef RAKSAMP_CLIENT
#define RAKSAMP_CLIENT
#endif
#define NETCODE_CONNCOOKIELULZ 0x6969

#include "source/vendor/raknet/SAMP/samp_netencr.h"
#include "source/vendor/raknet/SAMP/SAMPRPC.h"

void InitialiseClient();
void UninitialiseClient();
void ExitGame();