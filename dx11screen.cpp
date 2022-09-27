#include "dx11screen.h"

#ifdef Q_OS_WIN
//Constructor
DX11Screen::DX11Screen() : Screen()
{
    pDesktopWnd = GetDesktopWindow();
    pScreenSize = getWindowSize(pDesktopWnd);
    pBackBuffer = nullptr;
    pSwapChain = nullptr;
    pDeviceContext = nullptr;
    pDevice11 = nullptr;

    createVariables();
}

//Destructor
DX11Screen::~DX11Screen()
{
    //Clean up all variables
    pBackBuffer->Release();
    pSwapChain->Release();
    pDeviceContext->Release();
    pDevice11->Release();
}

//Main function to start capturing the screen
QImage DX11Screen::capture()
{
    //Check if everything is up and running, otherwise recreate
    createVariables();

    //Create new texture to copy to
    ID3D11Texture2D* pNewTexture = NULL;

    D3D11_TEXTURE2D_DESC description;
    pBackBuffer->GetDesc( &description );
    description.BindFlags = 0;//D3D11_BIND_RENDER_TARGET;
    description.MiscFlags = 0;//D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;// | D3D11_CPU_ACCESS_WRITE;
    description.Usage = D3D11_USAGE_STAGING;

    HRESULT hr = pDevice11->CreateTexture2D( &description, NULL, &pNewTexture );
    Q_ASSERT(SUCCEEDED(hr));
    if (!pNewTexture)
        return QImage();

    //Copy back buffer into new texture
    pDeviceContext->CopyResource(pNewTexture, pBackBuffer);

    D3D11_MAPPED_SUBRESOURCE resource;
    hr = pDeviceContext->Map(pNewTexture, 0/*subresource*/, D3D11_MAP_READ, 0, &resource);
    Q_ASSERT(SUCCEEDED(hr));

    //Create output image from the CPU available texture
    QImage outImage(pScreenSize.width, pScreenSize.height, QImage::Format_RGBA8888);
    memcpy(outImage.bits(), resource.pData, outImage.sizeInBytes());

    //Clean up
    pDeviceContext->Unmap(pNewTexture, 0);

    return outImage;
}

//Create all the variables (after screen size change)
void DX11Screen::createVariables()
{
    ScreenSize newSize = getWindowSize(pDesktopWnd);
    if (newSize == pScreenSize && pSwapChain && pBackBuffer)
        return;

    createSwapChain();
    createTexture();
}

//Create the swap chain
void DX11Screen::createSwapChain()
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

    //Release previous swap chain if we're recreating them
    if (pSwapChain)
        pSwapChain->Release();
    if (pDevice11)
        pDevice11->Release();
    if (pDeviceContext)
        pDeviceContext->Release();

    //Create swap chain and device
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &pScDesc, &pSwapChain, &pDevice11, NULL, &pDeviceContext);
    Q_ASSERT(SUCCEEDED(hr));
}

//Create the textures
void DX11Screen::createTexture()
{
    Q_ASSERT(pSwapChain);
    if (!pSwapChain)
        return;

    //Release previous back buffer if we need to recreate
    if (pBackBuffer)
        pBackBuffer->Release();

    //Create backbuffer texture
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pBackBuffer));
    Q_ASSERT(SUCCEEDED(hr));
}

#endif
