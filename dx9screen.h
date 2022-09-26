#pragma once

#include "screen.h"

class DX9Screen : public Screen
{
public:
    DX9Screen(const CaptureMode type, ScreenSize size);
    DX9Screen(const CaptureMode);
    ~DX9Screen();

    //Main function to return the captured screen
    QImage capture();
private:
    QImage pBuffer;
};

