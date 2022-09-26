#include "x11screen.h"

X11Screen::X11Screen(const CaptureMode type, ScreenSize size) : Screen(type, size)
{
    //Default constructor
}

X11Screen::X11Screen(const CaptureMode type) : Screen(type)
{
    //Convenience constructor to not set a screen size
}

X11Screen::~X11Screen()
{
    //Default destructor
}
