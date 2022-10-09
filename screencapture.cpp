#include "screencapture.h"

#ifdef Q_OS_WIN
#include <winuser.h>
#include <winerror.h>
#include <errhandlingapi.h>
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
    BOOL res = GetWindowRect(window, &rect);
    if (res == 0)
        return WindowSizeValue(Error("Error getting window size - %1").arg(HRESULT_FROM_WIN32(GetLastError())), ScreenSize());
    ScreenSize newSize = ScreenSize(qAbs(rect.bottom - rect.top), qAbs(rect.right - rect.left));

    return WindowSizeValue(NoError, newSize);
}
#endif

