#include "dxgiscreen.h"

DXGIScreen::DXGIScreen(const CaptureMode type, ScreenSize size) : Screen(type, size)
{
    //Default constructor
}

DXGIScreen::DXGIScreen(const CaptureMode type) : Screen(type)
{
    //Convenience constructor to not set a screen size
}

DXGIScreen::~DXGIScreen()
{
    //Default destructor
}
