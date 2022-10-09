#include "dx11screencapture.h"

#ifdef Q_OS_WIN
//Constructor
DX11ScreenCapture::DX11ScreenCapture() : ScreenCapture(), pDevice11(nullptr), pDeviceContext(nullptr), pSwapChain(nullptr), pBackBuffer(nullptr)
{
    pDesktopWnd = GetDesktopWindow();
    pScreenSize = getWindowSize(pDesktopWnd).value;

    createVariables();
}

//Destructor
DX11ScreenCapture::~DX11ScreenCapture()
{
    cleanup();
}

//Main function to start capturing the screen
CaptureValue DX11ScreenCapture::capture()
{
    //Check if everything is up and running, otherwise recreate
    Error createError = createVariables();
    if (createError != NoError)
        return CaptureValue(createError, pFrame);

    if (!pInitialised)
        //Not yet initialised - return current image
        return CaptureValue("DX11ScreenCapture: Not yet initialised", pFrame);

    //Create new texture to copy to
    ID3D11Texture2D* pNewTexture = nullptr;

    D3D11_TEXTURE2D_DESC description;
    pBackBuffer->GetDesc( &description );
    description.BindFlags = 0;//D3D11_BIND_RENDER_TARGET;
    description.MiscFlags = 0;//D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;// | D3D11_CPU_ACCESS_WRITE;
    description.Usage = D3D11_USAGE_STAGING;

    HRESULT hr = pDevice11->CreateTexture2D( &description, NULL, &pNewTexture );
    if (FAILED(hr) || !pNewTexture)
        return CaptureValue("DX11ScreenCapture: Create texture failed", pFrame);

    //Copy back buffer into new texture
    pDeviceContext->CopyResource(pNewTexture, pBackBuffer);

    D3D11_MAPPED_SUBRESOURCE resource;
    hr = pDeviceContext->Map(pNewTexture, 0/*subresource*/, D3D11_MAP_READ, 0, &resource);
    if (FAILED(hr))
        return CaptureValue("DX11ScreenCapture: Texture mapping failed", pFrame);

    //Create output image from the CPU available texture
    memcpy(pFrame.bits(), resource.pData, pFrame.sizeInBytes()); //TODO: bits() does deep-copy

    //Clean up
    pDeviceContext->Unmap(pNewTexture, 0);

    return CaptureValue(pFrame);
}

//Create all the variables (after screen size change)
Error DX11ScreenCapture::createVariables()
{
    //Get current window size
    WindowSizeValue newSize = getWindowSize(pDesktopWnd);
    if (newSize.failed())
        return newSize.error;

    //If everything is still the same, we're still good
    if (newSize.value == pScreenSize && pInitialised)
        return NoError;

    //Clean up and start from scratch
    cleanup();

    //Set new screen size
    pScreenSize = newSize.value;

    //Create image
    pFrame = QImage(pScreenSize.width, pScreenSize.height, QImage::Format_RGBA8888);

    //Create swap chain
    Error er = createSwapChain();
    if (er != NoError)
        return er;

    //Create texture
    er = createTexture();
    if (er != NoError)
        return er;

    pInitialised = true;
    return NoError;
}

//Create the swap chain
Error DX11ScreenCapture::createSwapChain()
{
    //Create swap chain descriptor
    ZeroMemory(&pScDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    pScDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pScDesc.BufferDesc.Width = pScreenSize.width;
    pScDesc.BufferDesc.Height = pScreenSize.height;
    pScDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;// DXGI_USAGE_RENDER_TARGET_OUTPUT;
    pScDesc.BufferCount = 1;
    pScDesc.OutputWindow = pDesktopWnd;
    pScDesc.Windowed = true;
    pScDesc.SampleDesc.Count = 1;

    //Create swap chain and device
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &pScDesc, &pSwapChain, &pDevice11, NULL, &pDeviceContext);
    if (FAILED(hr))
        return Error("DX11ScreenCapture: SwapChain creation failed with %1").arg(hr);
    return NoError;
}

//Create the textures
Error DX11ScreenCapture::createTexture()
{
    Q_ASSERT(pSwapChain);
    //Create backbuffer texture
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pBackBuffer));
    if (FAILED(hr))
        return Error("DX11ScreenCapture: Texture creation failed with %1").arg(hr);
    return NoError;
}

void DX11ScreenCapture::cleanup()
{
    //Clean up all variables
    if (pBackBuffer)
        pBackBuffer->Release();
    if (pSwapChain)
        pSwapChain->Release();
    if (pDeviceContext)
        pDeviceContext->Release();
    if (pDevice11)
        pDevice11->Release();

    pBackBuffer = nullptr;
    pSwapChain = nullptr;
    pDeviceContext = nullptr;
    pDevice11 = nullptr;
    pInitialised = false;
}

#endif
