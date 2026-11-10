#include "CLoadingScreen.h"
#include "../../Launcher/MGFlutterLoadingScreen.h"

void CLoadingScreen::SetInterface(bool state)
{
    MGSetLoadingScreenVisible(state);
}

void CLoadingScreen::UpdatePercent(int intMult)
{
    MGUpdateLoadingScreenPercent(intMult);
    if (intMult >= 100)
    {
        CLoadingScreen::SetInterface(false);
    }
}

void CLoadingScreen::UpdateList(const char* FileName)
{
    MGUpdateLoadingScreenList(FileName);
}