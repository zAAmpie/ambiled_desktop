#pragma once

#include "screen.h"

class X11Screen : public Screen
{
public:
    X11Screen(const CaptureMode type, ScreenSize size);
    X11Screen(const CaptureMode);
    ~X11Screen();

    //Main function to return the captured screen
    QImage capture();
private:
    QImage pBuffer;
};


