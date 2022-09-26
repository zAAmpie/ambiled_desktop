#pragma once

#include "screen.h"

class X11Screen : public Screen
{
public:
    X11Screen();
    ~X11Screen();

    //Get methods
    CaptureMode type() const {return Screen::X11Mode;}

    //Main function to start capturing the screen
    QImage capture();
};


