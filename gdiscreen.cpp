#include "gdiscreen.h"
#ifdef Q_OS_WIN
#include <winuser.h>
#include "util.h"

//Constructor
GDIScreen::GDIScreen() : Screen()
{
    //Construct all GDI variables
    pDesktopWnd = GetDesktopWindow();
    pDesktopDC = GetDC(pDesktopWnd);
    pCaptureDC = CreateCompatibleDC(pDesktopDC);

    pCaptureBitmap = nullptr;
    createBitmap();
}

//Destructor
GDIScreen::~GDIScreen()
{
    ReleaseDC(pDesktopWnd, pDesktopDC);
    DeleteDC(pCaptureDC);
    DeleteObject(pCaptureBitmap);
}

//Main function to start capturing the screen
QImage GDIScreen::capture()
{
    //Create or use existing bitmap (in case screen size has changed)
    createBitmap();

    QImage outputImage = QImage(pScreenSize.width, pScreenSize.height, QImage::Format_RGB32);

    //Capture primary screen
    BitBlt(pCaptureDC, 0, 0, pScreenSize.width, pScreenSize.height, pDesktopDC, 0, 0, SRCCOPY);//|CAPTUREBLT);

    //Copy HBITMAP to BYTE array
    BYTE *data = new BYTE[outputImage.sizeInBytes()];
    GetDIBits(pCaptureDC, pCaptureBitmap, 0, pScreenSize.height, data, &pInfo, DIB_RGB_COLORS);

    //Copy vertically flipped data into output image
    revmemcpy(outputImage.bits(), data, sizeof(data), outputImage.bytesPerLine());
    //Clean up
    delete[] data;

    return outputImage;
}

//Creates a bitmap that is sized to the current screen
void GDIScreen::createBitmap()
{
    //Get current desktop size
    ScreenSize newSize = getWindowSize(pDesktopWnd);
    Q_ASSERT(newSize.isSet());

    if (newSize == pScreenSize && pCaptureBitmap)
        //Bitmap is still the same size, so it's fine
        return;

    //Create new bitmap
    pScreenSize = newSize;
    pCaptureBitmap = CreateCompatibleBitmap(pDesktopDC, pScreenSize.width, pScreenSize.height);
    SelectObject(pCaptureDC, pCaptureBitmap);

    //And create the bitmap metadata
    ZeroMemory(&pInfo, sizeof(BITMAPINFO));
    pInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pInfo.bmiHeader.biWidth = pScreenSize.width;
    pInfo.bmiHeader.biHeight = pScreenSize.height;
    pInfo.bmiHeader.biPlanes = 1;
    pInfo.bmiHeader.biBitCount = 32;
}

#endif
