#include "dx11screen.h"

DX11Screen::DX11Screen(const CaptureMode type, ScreenSize size) : Screen(type, size)
{
    //Default constructor
}

DX11Screen::DX11Screen(const CaptureMode type) : Screen(type)
{
    //Convenience constructor to not set a screen size
}

DX11Screen::~DX11Screen()
{
    //Default destructor
}
