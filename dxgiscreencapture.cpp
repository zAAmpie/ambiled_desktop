#include "dxgiscreencapture.h"
#include "qdebug.h"

#ifdef Q_OS_WIN
//Constructor
DXGIScreenCapture::DXGIScreenCapture() : ScreenCapture(), pHaveFrameLock(false), pD3DDevice(nullptr), pD3DDeviceContext(nullptr), pDeskDupl(nullptr)
{
    pScreenSize = ScreenSize();
}

//Destructor
DXGIScreenCapture::~DXGIScreenCapture()
{
    cleanup();
}

//Main function to start capturing the screen
CaptureValue DXGIScreenCapture::capture()
{
    //Create variables - if necessary
    QString createError = createVariables();
    if (createError != NoError)
        return CaptureValue(createError, pFrame);

    if (!pInitialised)
        //Not yet initialised - return current image
        return CaptureValue("DXGIScreenCapture: Not yet initialised", pFrame);

    HRESULT hr = S_OK;

    // according to the docs, it's best for performance if we hang onto the frame for as long as possible,
    // and only release the previous frame immediately before acquiring the next one. Something about
    // the OS coalescing updates, so that it doesn't have to store them as distinct things.
    if (pHaveFrameLock)
    {
        pHaveFrameLock = false;
        pDeskDupl->ReleaseFrame();
    }

    IDXGIResource* deskRes = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    hr = pDeskDupl->AcquireNextFrame(0, &frameInfo, &deskRes);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        //Timed out, not actual error, return the previous frame
        return CaptureValue("DXGIScreenCapture: Timed out", pFrame);
    }

    if (FAILED(hr))
        return CaptureValue("DXGIScreenCapture: Acquire failed", pFrame);

    pHaveFrameLock = true;

    //Create GPU texture
    ID3D11Texture2D* gpuTex = nullptr;
    hr = deskRes->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&gpuTex);
    deskRes->Release();
    deskRes = nullptr;

    if (FAILED(hr))
        return CaptureValue("DXGIScreenCapture: QueryInterface failed", pFrame);

    //Create texture descriptor
    D3D11_TEXTURE2D_DESC desc;
    gpuTex->GetDesc(&desc);
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0; // D3D11_RESOURCE_MISC_GDI_COMPATIBLE ?
    desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
    ID3D11Texture2D* cpuTex = nullptr;
    hr = pD3DDevice->CreateTexture2D(&desc, nullptr, &cpuTex);

    if (FAILED(hr))
        return CaptureValue("DXGIScreenCapture: CreateTexture failed", pFrame);

    pD3DDeviceContext->CopyResource(cpuTex, gpuTex);

    //Map subresource
    D3D11_MAPPED_SUBRESOURCE sr;
    hr = pD3DDeviceContext->Map(cpuTex, 0, D3D11_MAP_READ, 0, &sr);

    if (FAILED(hr))
        return CaptureValue("DXGIScreenCapture: Map failed", pFrame);

    //Copy data to image
    ScreenSize newSize = ScreenSize(desc.Height, desc.Width);
    if (newSize != pScreenSize)
    {
        pFrame = QImage(newSize.width, newSize.height, QImage::Format_RGBA8888);
        pScreenSize = newSize;
    }
    memcpy(pFrame.bits(), sr.pData, pFrame.sizeInBytes());  //TODO: bits() does deep-copy

    pD3DDeviceContext->Unmap(cpuTex, 0);

    //Clean up
    cpuTex->Release();
    gpuTex->Release();

    return CaptureValue(pFrame);
}

//Create variables (typically after screen change)
Error DXGIScreenCapture::createVariables()
{
    if (pD3DDevice && pD3DDeviceContext && pDeskDupl && pInitialised)
        //Everything is still correct
        return NoError;

    //Clean up
    cleanup();

    //Driver types supported
    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    auto numDriverTypes = ARRAYSIZE(driverTypes);

    //Feature levels supported
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1 };
    auto numFeatureLevels = ARRAYSIZE(featureLevels);

    D3D_FEATURE_LEVEL featureLevel;

    //Create device
    HRESULT hr = S_OK;
    for (size_t i = 0; i < numDriverTypes; i++) {
        HRESULT hr = D3D11CreateDevice(nullptr, driverTypes[i], nullptr, 0, featureLevels, (UINT)numFeatureLevels,
            D3D11_SDK_VERSION, &pD3DDevice, &featureLevel, &pD3DDeviceContext);
        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return Error("DXGIScreenCapture: CreateDevice failed with %1").arg(hr);

    //Get DXGI device
    IDXGIDevice* dxgiDevice = nullptr;
    hr = pD3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr))
        return Error("DXGIScreenCapture: QueryInterface failed with %1").arg(hr);

    //Get DXGI adapter
    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
    dxgiDevice->Release();
    dxgiDevice = nullptr;
    if (FAILED(hr))
        return Error("DXGIScreenCapture: Could not get DXGI adapter - %1").arg(hr);

    //Get output
    IDXGIOutput* dxgiOutput = nullptr;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    dxgiAdapter->Release();
    dxgiAdapter = nullptr;
    if (FAILED(hr))
        return Error("DXGIScreenCapture: Could not enumerate outputs - %1").arg(hr);

    //Get DXGI output descriptor
    dxgiOutput->GetDesc(&pOutputDesc);

    //QueryInterface for Output 1
    IDXGIOutput1* dxgiOutput1 = nullptr;
    hr = dxgiOutput->QueryInterface(__uuidof(dxgiOutput1), (void**)&dxgiOutput1);
    dxgiOutput->Release();
    dxgiOutput = nullptr;
    if (FAILED(hr))
        return Error("DXGIScreenCapture: QueryInterface1 failed with %1").arg(hr);

    //Create desktop duplication
    try
    {
        hr = dxgiOutput1->DuplicateOutput(pD3DDevice, &pDeskDupl);
    }
    catch (...)
    {
        dxgiOutput1->Release();
        dxgiOutput1 = nullptr;
        return Error("DXGIScreenCapture: Exception during DuplicateOutput");
    }
    dxgiOutput1->Release();
    dxgiOutput1 = nullptr;
    if (FAILED(hr))
    {
        qDebug() << HRESULT_CODE(hr);
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
            //Too many desktop recorders active
            return Error("DXGIScreenCapture: Too many desktop recorders active");
        }
        return Error("DXGIScreenCapture: DuplicateOutput failed with %1").arg(hr);
    }

    pInitialised = true;
    return NoError;
}

//Clean up variables
void DXGIScreenCapture::cleanup()
{
    if (pDeskDupl)
        pDeskDupl->Release();
    if (pD3DDeviceContext)
        pD3DDeviceContext->Release();
    if (pD3DDevice)
        pD3DDevice->Release();

    pDeskDupl = nullptr;
    pD3DDeviceContext = nullptr;
    pD3DDevice = nullptr;
    pInitialised = false;
    pHaveFrameLock = false;
}

#endif
