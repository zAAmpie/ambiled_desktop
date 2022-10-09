#pragma once

#include "screencapture.h"
class GDIScreenCapture;

#ifdef Q_OS_WIN
#include <windef.h>
#include <wingdi.h>

class GDIScreenCapture : public ScreenCapture
{
public:
    GDIScreenCapture();
    ~GDIScreenCapture();

    //Get methods
    CaptureMode type() const override {return ScreenCapture::GDIMode;}

    //Main function to start capturing the screen
    CaptureValue capture() override;
private:
    //Creates a bitmap that is sized to the current screen
    Error createVariables();

    //Clean up
    void cleanup();

    HWND pDesktopWnd;
    HDC pDesktopDC;
    HDC pCaptureDC;
    HBITMAP pCaptureBitmap;
    BITMAPINFO pInfo;
};

#endif
