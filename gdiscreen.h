#pragma once

#include "screen.h"

class GDIScreen : public Screen
{
public:
    GDIScreen();
    ~GDIScreen();

    //Get methods
    CaptureMode type() const {return Screen::GDIMode;}

    //Main function to start capturing the screen
    QImage capture();
};


