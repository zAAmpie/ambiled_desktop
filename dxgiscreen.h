#pragma once

#include "screen.h"

class DXGIScreen : public Screen
{
public:
    DXGIScreen();
    ~DXGIScreen();

    //Get methods
    CaptureMode type() const {return Screen::DXGIMode;}

    //Main function to start capturing the screen
    QImage capture();
};

