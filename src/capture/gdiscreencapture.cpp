#include "gdiscreencapture.h"
#ifdef Q_OS_WIN
#include <winuser.h>
#include "util.h"

//Constructor
GDIScreenCapture::GDIScreenCapture() : ScreenCapture(), pDesktopWnd(nullptr), pDesktopDC(nullptr), pCaptureDC(nullptr), pCaptureBitmap(nullptr)
{
    pScreenSize = ScreenSize();
}

//Destructor
GDIScreenCapture::~GDIScreenCapture()
{
    cleanup();
}

//Main function to start capturing the screen
CaptureValue GDIScreenCapture::capture()
{
    //Create or use existing bitmap (in case screen size has changed)
    Error createError = createVariables();
    if (createError != NoError)
        return CaptureValue(createError, pFrame);

    //QImage outputImage = QImage(pScreenSize.width, pScreenSize.height, QImage::Format_RGB32);

    //Capture primary screen
    BitBlt(pCaptureDC, 0, 0, pScreenSize.width, pScreenSize.height, pDesktopDC, 0, 0, SRCCOPY);//|CAPTUREBLT);

    //Copy HBITMAP to BYTE array
    BYTE *data = new BYTE[pScreenSize.totalBytes()];
    GetDIBits(pCaptureDC, pCaptureBitmap, 0, pScreenSize.height, data, &pInfo, DIB_RGB_COLORS);

    //Copy vertically flipped data into output image
    revmemcpy(pFrame.bits(), data, pScreenSize.totalBytes(), pScreenSize.bytesPerLine());  //TODO: bits() does deep-copy
    //memcpy(pBuffer, data, pScreenSize.totalBytes());
    //Clean up
    delete[] data;

    //return QImage(pBuffer, pScreenSize.width, pScreenSize.height, QImage::Format_RGB32);
    return CaptureValue(pFrame);
}

//Creates a bitmap that is sized to the current screen
Error GDIScreenCapture::createVariables()
{
    //If already initialised, check if everything is still as expected
    if (pInitialised)
    {
        //Get current desktop size
        WindowSizeValue newSize = getWindowSize(pDesktopWnd);
        if (newSize.error != NoError)
            return newSize.error;

        if (newSize.value == pScreenSize && pCaptureBitmap)
            //Bitmap is still the same size, so it's fine
            return NoError;
    }

    //Start from scratch
    cleanup();

    //Create GDI variables
    pDesktopWnd = GetDesktopWindow();
    pDesktopDC = GetDC(pDesktopWnd);
    pCaptureDC = CreateCompatibleDC(pDesktopDC);

    //Get current desktop size
    WindowSizeValue newSize = getWindowSize(pDesktopWnd);
    if (newSize.error != NoError)
        return newSize.error;

    //Reset screen size
    pScreenSize = newSize.value;

    //Resize buffer
    pFrame = QImage(pScreenSize.width, pScreenSize.height, QImage::Format_RGB32);

    //Create new bitmap
    pCaptureBitmap = CreateCompatibleBitmap(pDesktopDC, pScreenSize.width, pScreenSize.height);
    SelectObject(pCaptureDC, pCaptureBitmap);

    //And create the bitmap metadata
    ZeroMemory(&pInfo, sizeof(BITMAPINFO));
    pInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pInfo.bmiHeader.biWidth = pScreenSize.width;
    pInfo.bmiHeader.biHeight = pScreenSize.height;
    pInfo.bmiHeader.biPlanes = 1;
    pInfo.bmiHeader.biBitCount = 32;

    pInitialised = true;
    return NoError;
}

//Clean up
void GDIScreenCapture::cleanup()
{
    ReleaseDC(pDesktopWnd, pDesktopDC);
    DeleteDC(pCaptureDC);
    DeleteObject(pCaptureBitmap);
    pDesktopWnd = nullptr;
    pDesktopDC = nullptr;
    pCaptureDC = nullptr;
    pCaptureBitmap = nullptr;
    pInitialised = false;
}

#endif
