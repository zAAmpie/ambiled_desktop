#include "x11screencapture.h"

#ifdef Q_OS_UNIX
//Constructor
X11ScreenCapture::X11ScreenCapture() : ScreenCapture()
{
    pDisplay = nullptr;
    pScreen = nullptr;
    pXImg = nullptr;
    pScreenSize = ScreenSize();
}

//Destructor
X11ScreenCapture::~X11ScreenCapture()
{
    //Remove image
   XDestroyImage(pXImg);
   //Detach image from X11
   XShmDetach(pDisplay, &pShmInfo);
   //Delete shared memory
   shmdt(pShmInfo.shmaddr);
   //Close display
   XCloseDisplay(pDisplay);
}

//Main function to start capturing the screen
QImage X11ScreenCapture::capture()
{
    //Create if needed
    createVariables();

    //Grab new image from X11
    XShmGetImage(pDisplay, pRoot, pXImg, 0, 0, 0x00ffffff);

    //Copy to output image
    QImage outImage = QImage(reinterpret_cast<BYTE *>(pXImg->data), pScreenSize.width, pScreenSize.height, QImage::Format_RGB32);

    return outImage;
}

//Create the requisite variables
void X11ScreenCapture::createVariables()
{
    if (pDisplay && pScreen && pXImg)
        //Everything seems to be fine
        return;

    //Open display and set to root window
    pDisplay = XOpenDisplay(nullptr);
    pRoot = DefaultRootWindow(pDisplay);

    //Get display attributes
    XGetWindowAttributes(pDisplay, pRoot, &pWindowAttributes);
    pScreen = pWindowAttributes.screen;
    pScreenSize = ScreenSize(pWindowAttributes.height, pWindowAttributes.width);
    Q_ASSERT(pScreenSize.isSet());

    //Create shared memory space for display - These are C functions so need to deal with some compiler warnings.
    pXImg = XShmCreateImage(pDisplay, DefaultVisualOfScreen(pScreen), DefaultDepthOfScreen(pScreen), ZPixmap, nullptr, &pShmInfo, static_cast<unsigned int>(pScreenSize.width), static_cast<unsigned int>(pScreenSize.height));

    //Assign attributes
    pShmInfo.shmid = shmget(IPC_PRIVATE, static_cast<size_t>(pXImg->bytes_per_line * pXImg->height), IPC_CREAT|0777);
    pShmInfo.shmaddr = pXImg->data = static_cast<char*>(shmat(pShmInfo.shmid, nullptr, 0));
    pShmInfo.readOnly = false;
    Q_ASSERT(pShmInfo.shmid >= 0);

    //Attach memory to X11
    int result = XShmAttach(pDisplay, &pShmInfo);
    Q_ASSERT(result);
}
#endif
