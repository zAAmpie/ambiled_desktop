#include "x11screencapture.h"

#ifdef Q_OS_UNIX
//Constructor
X11ScreenCapture::X11ScreenCapture() : ScreenCapture(), pDisplay(nullptr), pScreen(nullptr), pXImg(nullptr)
{
    pScreenSize = ScreenSize();
}

//Destructor
X11ScreenCapture::~X11ScreenCapture()
{
    cleanup();
}

//Main function to start capturing the screen
CaptureValue X11ScreenCapture::capture()
{
    //Create variables - if necessary
    QString createError = createVariables();
    if (createError != NoError)
        return CaptureValue(createError, pFrame);

    if (!pInitialised)
        //Not yet initialised - return current image
        return CaptureValue("X11ScreenCapture: Not yet initialised", pFrame);

    //Grab new image from X11
    XShmGetImage(pDisplay, pRoot, pXImg, 0, 0, 0x00ffffff);

    //Copy to output image
    memcpy(pFrame.bits(), reinterpret_cast<BYTE *>(pXImg->data), pFrame.sizeInBytes());

    return CaptureValue(pFrame);
}

//Create the requisite variables
Error X11ScreenCapture::createVariables()
{
    if (pDisplay && pScreen && pXImg && pInitialised)
        //Everything seems to be fine
        return NoError;

    //Open display and set to root window
    pDisplay = XOpenDisplay(nullptr);
    pRoot = DefaultRootWindow(pDisplay);

    //Get display attributes
    XGetWindowAttributes(pDisplay, pRoot, &pWindowAttributes);
    pScreen = pWindowAttributes.screen;
    pScreenSize = ScreenSize(pWindowAttributes.height, pWindowAttributes.width);
    if (!pScreenSize.isSet())
        return Error("X11ScreenCapture: Could not get window size");

    //Create shared memory space for display - These are C functions so need to deal with some compiler warnings.
    pXImg = XShmCreateImage(pDisplay, DefaultVisualOfScreen(pScreen), DefaultDepthOfScreen(pScreen), ZPixmap, nullptr, &pShmInfo, static_cast<unsigned int>(pScreenSize.width), static_cast<unsigned int>(pScreenSize.height));

    //Assign attributes
    pShmInfo.shmid = shmget(IPC_PRIVATE, static_cast<size_t>(pXImg->bytes_per_line * pXImg->height), IPC_CREAT|0777);
    pShmInfo.shmaddr = pXImg->data = static_cast<char*>(shmat(pShmInfo.shmid, nullptr, 0));
    pShmInfo.readOnly = false;
    if (pShmInfo.shmid < 0)
        return Error("X11ScreenCapture: Could not assign X11 attributes");

    //Attach memory to X11
    int result = XShmAttach(pDisplay, &pShmInfo);
    if (!result)
        return Error("X11ScreenCapture: Could not attach to display shared memory");

    pInitialised = true;
    return NoError;
}

//Clean up variables
void X11ScreenCapture::cleanup()
{
    //Remove image
   XDestroyImage(pXImg);
   //Detach image from X11
   XShmDetach(pDisplay, &pShmInfo);
   //Delete shared memory
   shmdt(pShmInfo.shmaddr);
   //Close display
   XCloseDisplay(pDisplay);

   //Standard X11 variables
   pDisplay = nullptr;
   pScreen = nullptr;
   pXImg = nullptr;
   pInitialised = false;
}

#endif
