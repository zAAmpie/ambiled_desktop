#include "gdiscreen.h"

GDIScreen::GDIScreen(const CaptureMode type, ScreenSize size) : Screen(type, size)
{
    //Default constructor
}

GDIScreen::GDIScreen(const CaptureMode type) : Screen(type)
{
    //Convenience constructor to not set a screen size
}

GDIScreen::~GDIScreen()
{
    //Default destructor
}
