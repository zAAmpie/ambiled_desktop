#include "x11screencapture.h"

#ifdef Q_OS_UNIX
#include "imageutils.h"

//Constructor
X11ScreenCapture::X11ScreenCapture() : ScreenCapture(), pDisplay(nullptr), pScreen(nullptr), pXImg(nullptr), pShmInfo(), pShmAttached(false)
{
    pScreenSize = ScreenSize();
    pShmInfo.shmid = -1;
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

    //Grab new image from X11 into the shared memory segment
    if (!XShmGetImage(pDisplay, pRoot, pXImg, 0, 0, 0x00ffffff))
        return CaptureValue(Error("X11ScreenCapture: XShmGetImage failed"), pFrame);

    //Copy out of the shared segment, downsampling as we go. A fresh image every time, so a frame
    //still referenced downstream is never written to (or deep-copied by detaching).
    pFrame = ImageUtils::downsample32(reinterpret_cast<const uchar *>(pXImg->data), pXImg->width, pXImg->height,
                                      pXImg->bytes_per_line, QImage::Format_RGB32, DownsampleFactor);

    return CaptureValue(pFrame);
}

//Create the requisite variables
Error X11ScreenCapture::createVariables()
{
    if (pInitialised)
        //Everything seems to be fine
        return NoError;

    //Release anything left over from a previous, failed attempt
    cleanup();

    //Open display and set to root window
    pDisplay = XOpenDisplay(nullptr);
    if (!pDisplay)
        return Error("X11ScreenCapture: Could not open X display");
    if (!XShmQueryExtension(pDisplay))
        return Error("X11ScreenCapture: X server does not support the MIT-SHM extension");
    pRoot = DefaultRootWindow(pDisplay);

    //Get display attributes
    XGetWindowAttributes(pDisplay, pRoot, &pWindowAttributes);
    pScreen = pWindowAttributes.screen;
    pScreenSize = ScreenSize(pWindowAttributes.height, pWindowAttributes.width);
    if (!pScreenSize.isSet())
        return Error("X11ScreenCapture: Could not get window size");

    //Create shared memory space for display - These are C functions so need to deal with some compiler warnings.
    pXImg = XShmCreateImage(pDisplay, DefaultVisualOfScreen(pScreen), DefaultDepthOfScreen(pScreen), ZPixmap, nullptr, &pShmInfo, static_cast<unsigned int>(pScreenSize.width), static_cast<unsigned int>(pScreenSize.height));
    if (!pXImg)
        return Error("X11ScreenCapture: Could not create shared memory image");
    if (pXImg->bits_per_pixel != 32)
        return Error("X11ScreenCapture: Unsupported pixel format (%1 bits per pixel)").arg(pXImg->bits_per_pixel);

    //Owner-only permissions - the segment holds the screen contents, so it mustn't be readable by other users
    pShmInfo.shmid = shmget(IPC_PRIVATE, static_cast<size_t>(pXImg->bytes_per_line * pXImg->height), IPC_CREAT | 0600);
    if (pShmInfo.shmid < 0)
        return Error("X11ScreenCapture: Could not allocate shared memory");
    pShmInfo.shmaddr = pXImg->data = static_cast<char*>(shmat(pShmInfo.shmid, nullptr, 0));
    if (pShmInfo.shmaddr == reinterpret_cast<char *>(-1))
    {
        pShmInfo.shmaddr = pXImg->data = nullptr;
        return Error("X11ScreenCapture: Could not attach shared memory");
    }
    pShmInfo.readOnly = false;

    //Attach memory to X11
    if (!XShmAttach(pDisplay, &pShmInfo))
        return Error("X11ScreenCapture: Could not attach to display shared memory");
    pShmAttached = true;

    //Once the server has attached, mark the segment for removal: it stays alive until both sides detach,
    //but is then freed by the kernel even if we crash, rather than leaking one segment per run
    XSync(pDisplay, false);
    shmctl(pShmInfo.shmid, IPC_RMID, nullptr);

    pInitialised = true;
    return NoError;
}

//Clean up variables - safe to call on partially (or never) initialised state
void X11ScreenCapture::cleanup()
{
    //Detach image from X11
    if (pShmAttached)
        XShmDetach(pDisplay, &pShmInfo);
    //Remove image (XShm images don't own their data, so this leaves the shared segment alone)
    if (pXImg)
        XDestroyImage(pXImg);
    //Delete shared memory
    if (pShmInfo.shmaddr)
        shmdt(pShmInfo.shmaddr);
    if (pShmInfo.shmid >= 0)
        shmctl(pShmInfo.shmid, IPC_RMID, nullptr);
    //Close display
    if (pDisplay)
        XCloseDisplay(pDisplay);

    //Standard X11 variables
    pDisplay = nullptr;
    pScreen = nullptr;
    pXImg = nullptr;
    pShmInfo = XShmSegmentInfo();
    pShmInfo.shmid = -1;
    pShmAttached = false;
    pInitialised = false;
}

#endif
