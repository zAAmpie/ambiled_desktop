#pragma once

#include <QImage>
#include "types.h"

class Screen
{
    /* Base class for all screen capture types */
public:
    enum CaptureMode {GDIMode, DirectX9Mode, DirectX11Mode, DXGIMode, X11Mode};

    Screen();
    ~Screen();

    //Get methods
    virtual CaptureMode type() const;
    ScreenSize size() const {return pScreenSize;}

    //Main function to start capturing the screen
    virtual QImage capture();
protected:
    ScreenSize pScreenSize;
};

