#pragma once

#include "screen.h"

class GDIScreen : public Screen
{
public:
    GDIScreen(const CaptureMode type, ScreenSize size);
    GDIScreen(const CaptureMode);
    ~GDIScreen();

    //Main function to return the captured screen
    QImage capture();
private:
    QImage pBuffer;
};


