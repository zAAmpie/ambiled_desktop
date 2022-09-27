#pragma once

#include <QImage>
#include "types.h"
#ifdef Q_OS_WIN
#include <windef.h>
#endif

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
#ifdef Q_OS_WIN
    //Calculate window size
    ScreenSize getWindowSize(HWND &window);
#endif

    ScreenSize pScreenSize;
};

