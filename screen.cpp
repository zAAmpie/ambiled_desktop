#include "screen.h"

Screen::Screen(const CaptureMode type, ScreenSize size) : pType(type), pScreenSize(size)
{
    //Default constructor
}

Screen::Screen(const CaptureMode type) : pType(type), pScreenSize(ScreenSize(0,0))
{
    //Convenience constructor to not set a screen size
}

Screen::~Screen()
{
    //Default destructor
}
