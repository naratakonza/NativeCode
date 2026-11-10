#include "../../../../Main.h"
#include "CFastLoad.h"

void CFastLoad::Initialise()
{
    CNecessary::Log(OBFUSCATE("CFastLoad Initialise"));

	CMemory::makeRET(SA_ADDR(0x21907C)); // CLoadingScreen::DisplayPCScreen
}