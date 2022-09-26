#pragma once

#include "screen.h"

class DXGIScreen : public Screen
{
public:
    DXGIScreen(const CaptureMode type, ScreenSize size);
    DXGIScreen(const CaptureMode);
    ~DXGIScreen();

    //Main function to return the captured screen
    QImage capture();
private:
    QImage pBuffer;
};

