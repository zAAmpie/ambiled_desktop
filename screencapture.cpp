#include "screencapture.h"

#ifdef Q_OS_WIN
#include <winuser.h>
#endif

//Constructor
ScreenCapture::ScreenCapture() : pScreenSize(ScreenSize())
{

}

//Destructor
ScreenCapture::~ScreenCapture()
{

}


#ifdef Q_OS_WIN
//Calculate window size
ScreenSize ScreenCapture::getWindowSize(HWND &window)
{
    //Get current window size
    RECT rect;
    bool res = GetWindowRect(window, &rect);
    Q_ASSERT(res);
    ScreenSize newSize = ScreenSize(qAbs(rect.bottom - rect.top), qAbs(rect.right - rect.left));

    return newSize;
}
#endif
