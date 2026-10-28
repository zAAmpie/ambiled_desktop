#pragma once

#include <QImage>
#include "types.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

using CaptureValue = ReturnValue<QImage>;
using WindowSizeValue = ReturnValue<ScreenSize>;

class ScreenCapture
{
    /* Base class for all screen capture types */
public:
    enum CaptureMode {GDIMode, DirectX9Mode, DirectX11Mode, DXGIMode, X11Mode, WaylandMode};

    ScreenCapture();
    virtual ~ScreenCapture();

    //Get methods
    virtual CaptureMode type() const = 0;
    ScreenSize size() const {return pScreenSize;}

    //Main function to start capturing the screen
    virtual CaptureValue capture() = 0;

    //Where the captured output sits in the desktop's (logical) coordinates, if the backend knows -
    //used to show test patterns on the captured screen. Empty if unknown (e.g. X11 captures the
    //whole root window, which can span several screens).
    virtual QRect outputGeometry() const {return QRect();}

    //Backends that downsample while copying out of the system's frame buffer (Wayland, X11) make
    //frames this many times smaller in each dimension (power of two) - see ImageUtils::downsample32
    static constexpr int DownsampleFactor = 4;
protected:
#ifdef Q_OS_WIN
    //Calculate window size
    WindowSizeValue getWindowSize(HWND &window);
#endif

    ScreenSize pScreenSize;
    QImage pFrame;
    bool pInitialised;
};

