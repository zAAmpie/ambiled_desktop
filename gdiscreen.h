#pragma once

#include "screen.h"
class GDIScreen;

#ifdef Q_OS_WIN
#include <windef.h>
#include <wingdi.h>

class GDIScreen : public Screen
{
public:
    GDIScreen();
    ~GDIScreen();

    //Get methods
    CaptureMode type() const override {return Screen::GDIMode;}

    //Main function to start capturing the screen
    QImage capture() override;
private:
    //Creates a bitmap that is sized to the current screen
    void createBitmap();

    //Calculate window size
    ScreenSize getWindowSize(HWND &window);

    HWND pDesktopWnd;
    HDC pDesktopDC;
    HDC pCaptureDC;
    HBITMAP pCaptureBitmap;
    BITMAPINFO pInfo;
};

#endif
