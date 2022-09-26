#pragma once

#include "screen.h"

class DX9Screen : public Screen
{
public:
    DX9Screen();
    ~DX9Screen();

    //Get methods
    CaptureMode type() const {return Screen::DirectX9Mode;}

    //Main function to start capturing the screen
    QImage capture();
};

