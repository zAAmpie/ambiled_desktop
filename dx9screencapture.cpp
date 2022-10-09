#include "dx9screencapture.h"
#include "util.h"

#ifdef Q_OS_WIN
//Constructor
DX9ScreenCapture::DX9ScreenCapture() : ScreenCapture(), pD3d(nullptr), pDevice(nullptr), pSurface(nullptr)
{
    pDesktopWnd = GetDesktopWindow();
    pScreenSize = getWindowSize(pDesktopWnd).value;
}

//Destructor
DX9ScreenCapture::~DX9ScreenCapture()
{
    cleanup();
}

//Main function to start capturing the screen
CaptureValue DX9ScreenCapture::capture()
{
    //Check if everything is up and running, otherwise recreate
    Error createError = createVariables();
    if (createError != NoError)
        return CaptureValue(createError, pFrame);

    if (!pInitialised)
        //Not yet initialised - return current image
        return CaptureValue("DX9ScreenCapture: Not yet initialised", pFrame);

    //Copy front buffer to surface
    pDevice->GetFrontBufferData(0, pSurface);

    //Lock front buffer
    D3DLOCKED_RECT lockedRect;
    HRESULT hr = pSurface->LockRect(&lockedRect, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
    if (FAILED(hr))
        return CaptureValue("DX9ScreenCapture: Could not lock front buffer", pFrame);

    //Copy locked rectangle to byte array (flip vertically)
    revmemcpy(pFrame.bits(), lockedRect.pBits, pFrame.sizeInBytes(), pFrame.bytesPerLine());  //TODO: bits() does deep-copy

    //Unlock rect
    pSurface->UnlockRect();

    return CaptureValue(pFrame);
}

//Create variables (typically after screen change)
Error DX9ScreenCapture::createVariables()
{
    //Get current window size
    WindowSizeValue newSize = getWindowSize(pDesktopWnd);
    if (newSize.failed())
        return newSize.error;

    //If everything is still the same, we're still good
    if (newSize.value == pScreenSize && pInitialised)
        return NoError;

    //Clean up previous variables
    cleanup();

    //Set new screen size
    pScreenSize = newSize.value;

    //Create image
    pFrame = QImage(pScreenSize.width, pScreenSize.height, QImage::Format_RGBA8888);

    //Create D3D device object
    pD3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3d)
        return Error("DX9ScreenCapture: Could not create Direct3D device");

    //Create D3D descriptor
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(D3DPRESENT_PARAMETERS));
    d3dpp.Windowed=true;
    d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferHeight = pScreenSize.height;
    d3dpp.BackBufferWidth = pScreenSize.width;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = pDesktopWnd;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

    //Create/initalize device
    HRESULT hr = pD3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pDesktopWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
    if (FAILED(hr))
        return Error("DX9ScreenCapture: Could not create DirectX9 device - %1").arg(hr);

    //Create offscreen surface
    hr = pDevice->CreateOffscreenPlainSurface(pScreenSize.width, pScreenSize.height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &pSurface, NULL);
    if (FAILED(hr))
        return Error("DX9ScreenCapture: Could not create offscreen surface - %1").arg(hr);

    pInitialised = true;
    return NoError;
}

//Clean up variables
void DX9ScreenCapture::cleanup()
{
    if (pSurface)
        pSurface->Release();
    if (pDevice)
        pDevice->Release();
    if (pD3d)
        pD3d->Release();

    pSurface = nullptr;
    pDevice = nullptr;
    pD3d = nullptr;
    pInitialised = false;
}

#endif
