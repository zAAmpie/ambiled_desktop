#include "dx9screen.h"

DX9Screen::DX9Screen(const CaptureMode type, ScreenSize size) : Screen(type, size)
{
    //Default constructor
}

DX9Screen::DX9Screen(const CaptureMode type) : Screen(type)
{
    //Convenience constructor to not set a screen size
}

DX9Screen::~DX9Screen()
{
    //Default destructor
}
