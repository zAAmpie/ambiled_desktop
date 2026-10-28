#pragma once

#include "screencapture.h"
class X11ScreenCapture;

#ifdef Q_OS_UNIX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//Hack to undefine some standard definitions from X11 messing with other libraries
#undef Bool
#undef Void
#undef Status
#undef None

class X11ScreenCapture : public ScreenCapture
{
public:
    X11ScreenCapture();
    ~X11ScreenCapture();

    //Get methods
    CaptureMode type() const override {return ScreenCapture::X11Mode;}

    //Main function to start capturing the screen - grabs the root window via MIT-SHM and returns it
    //downsampled by DownsampleFactor in each dimension
    CaptureValue capture() override;
private:
    //Create the requisite variables
    Error createVariables();

    //Clean up variables - safe to call on partially (or never) initialised state
    void cleanup();

    //Standard X11 variables
    Display* pDisplay;
    Window pRoot;
    XWindowAttributes pWindowAttributes;
    Screen* pScreen;

    //Per capture data and information
    XImage* pXImg;
    XShmSegmentInfo pShmInfo;
    bool pShmAttached; //Whether the X server has attached pShmInfo (and so must be detached in cleanup)
};

#endif
