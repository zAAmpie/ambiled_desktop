#include "dx9screencapture.h"
#include "util.h"

#ifdef Q_OS_WIN
//Constructor
DX9ScreenCapture::DX9ScreenCapture() : ScreenCapture()
{
    pDesktopWnd = GetDesktopWindow();
    pScreenSize = getWindowSize(pDesktopWnd);
    pD3d = nullptr;
    pDevice = nullptr;
    pSurface = nullptr;
}

//Destructor
DX9ScreenCapture::~DX9ScreenCapture()
{
    pSurface->Release();
    pDevice->Release();
    pD3d->Release();
}

//Main function to start capturing the screen
QImage DX9ScreenCapture::capture()
{
    //Check if everything is still good, otherwise recreate
    createVariables();

    //Copy front buffer to surface
    pDevice->GetFrontBufferData(0, pSurface);

    //Lock front buffer
    D3DLOCKED_RECT lockedRect;
    HRESULT hr = pSurface->LockRect(&lockedRect, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
    Q_ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return QImage();

    //Create output image
    QImage outImage = QImage(pScreenSize.width, pScreenSize.height, QImage::Format_RGB32);

    //Copy locked rectangle to byte array (flip vertically)
    revmemcpy(outImage.bits(), lockedRect.pBits, outImage.sizeInBytes(), outImage.bytesPerLine());

    //Unlock rect
    pSurface->UnlockRect();

    return outImage;
}

//Create variables (typically after screen change)
void DX9ScreenCapture::createVariables()
{
    //Check if screen size has changed
    ScreenSize newSize = getWindowSize(pDesktopWnd);
    if (newSize == pScreenSize && pD3d && pDevice && pSurface)
        //All still good, nothing to create
        return;

    //Release previously created objects if necessary
    if (pD3d)
        pD3d->Release();
    if (pDevice)
        pDevice->Release();
    if (pSurface)
        pSurface->Release();

    //Create D3D device object
    pD3d = Direct3DCreate9(D3D_SDK_VERSION);
    Q_ASSERT(pD3d);
    if (!pD3d)
        return;

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
    Q_ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return;

    //Create offscreen surface
    hr = pDevice->CreateOffscreenPlainSurface(pScreenSize.width, pScreenSize.height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &pSurface, NULL);
    Q_ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return;
}

#endif
