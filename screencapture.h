#pragma once

#include <QImage>
#include "types.h"
#ifdef Q_OS_WIN
#include <windef.h>
#endif

using CaptureValue = ReturnValue<QImage>;
using WindowSizeValue = ReturnValue<ScreenSize>;

class ScreenCapture
{
    /* Base class for all screen capture types */
public:
    enum CaptureMode {GDIMode, DirectX9Mode, DirectX11Mode, DXGIMode, X11Mode};

    ScreenCapture();
    virtual ~ScreenCapture();

    //Get methods
    virtual CaptureMode type() const = 0;
    ScreenSize size() const {return pScreenSize;}

    //Main function to start capturing the screen
    virtual CaptureValue capture() = 0;
protected:
#ifdef Q_OS_WIN
    //Calculate window size
    WindowSizeValue getWindowSize(HWND &window);
#endif

    ScreenSize pScreenSize;
    QImage pFrame;
    bool pInitialised;
};

