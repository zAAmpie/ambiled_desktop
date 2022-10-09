#include "screencapture.h"

#ifdef Q_OS_WIN
#include <winuser.h>
#endif

//Constructor
ScreenCapture::ScreenCapture() : pScreenSize(ScreenSize()), pInitialised(false)
{

}

//Destructor
ScreenCapture::~ScreenCapture()
{

}


#ifdef Q_OS_WIN
//Calculate window size
WindowSizeValue ScreenCapture::getWindowSize(HWND &window)
{
    //Get current window size
    RECT rect;
    bool res = GetWindowRect(window, &rect);
    if (!res)
        return WindowSizeValue("Error getting window size", ScreenSize());
    ScreenSize newSize = ScreenSize(qAbs(rect.bottom - rect.top), qAbs(rect.right - rect.left));

    return WindowSizeValue(NoError, newSize);
}
#endif

