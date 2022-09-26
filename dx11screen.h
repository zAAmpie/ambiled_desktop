#pragma once

#include "screen.h"

class DX11Screen : public Screen
{
public:
    DX11Screen();
    ~DX11Screen();

    //Get methods
    CaptureMode type() const {return Screen::DirectX11Mode;}

    //Main function to start capturing the screen
    QImage capture();
};

