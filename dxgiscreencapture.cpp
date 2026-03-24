#include "dxgiscreencapture.h"
#include <QDebug>

#ifdef Q_OS_WIN
//Constructor
DXGIScreenCapture::DXGIScreenCapture() : ScreenCapture(), pHaveFrameLock(false), pD3DDevice(nullptr), pD3DDeviceContext(nullptr), pDeskDupl(nullptr), pCpuTex(nullptr), pCpuTexWidth(0), pCpuTexHeight(0), pScaleFactor(1.0f)
{
    pScreenSize = ScreenSize();
}

void DXGIScreenCapture::setGPUScaleFactor(float factor)
{
    pScaleFactor = factor;
    if (pScaler)
        pScaler->setScaleFactor(factor);
}

float DXGIScreenCapture::scaleFactor() const
{
    return pScaleFactor;
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
        //return CaptureValue("DXGIScreenCapture: Timed out", pFrame);
        return pFrame;
    }

    if (FAILED(hr))
    {
        pInitialised = false;
        return CaptureValue(Error("DXGIScreenCapture: Acquire failed with code %1").arg(HRESULT_CODE(hr)), pFrame);
    }

    pHaveFrameLock = true;

    //Create GPU texture
    ID3D11Texture2D* gpuTex = nullptr;
    hr = deskRes->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&gpuTex));
    deskRes->Release();
    deskRes = nullptr;

    if (FAILED(hr))
    {
        pInitialised = false;
        return CaptureValue(Error("DXGIScreenCapture: QueryInterface failed with code %1").arg(HRESULT_CODE(hr)), pFrame);
    }

    // GPU downscale: scale the captured texture on the GPU before readback.
    // Skip when scale factor is 1.0 (no downscale) or scaler is unavailable.
    ID3D11Texture2D* readbackTex = (pScaler && pScaleFactor < 1.0f) ? pScaler->scale(gpuTex) : nullptr;
    if (!readbackTex)
        readbackTex = gpuTex;

    D3D11_TEXTURE2D_DESC desc;
    readbackTex->GetDesc(&desc);

    //Recreate staging texture only when resolution changes, not every frame
    if (!pCpuTex || desc.Width != pCpuTexWidth || desc.Height != pCpuTexHeight)
    {
        if (pCpuTex)
            pCpuTex->Release();

        D3D11_TEXTURE2D_DESC stagingDesc = desc;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.MiscFlags = 0;
        hr = pD3DDevice->CreateTexture2D(&stagingDesc, nullptr, &pCpuTex);

        if (FAILED(hr))
        {
            pCpuTex = nullptr;
            pInitialised = false;
            gpuTex->Release();
            return CaptureValue(Error("DXGIScreenCapture: CreateTexture failed with code %1").arg(HRESULT_CODE(hr)), pFrame);
        }

        pCpuTexWidth = desc.Width;
        pCpuTexHeight = desc.Height;
    }

    pD3DDeviceContext->CopyResource(pCpuTex, readbackTex);
    gpuTex->Release(); // scaler owns pOutputTex; we only release the original frame texture

    //Map subresource
    D3D11_MAPPED_SUBRESOURCE sr;
    hr = pD3DDeviceContext->Map(pCpuTex, 0, D3D11_MAP_READ, 0, &sr);

    if (FAILED(hr))
    {
        pInitialised = false;
        return CaptureValue(Error("DXGIScreenCapture: Map failed with code %1").arg(HRESULT_CODE(hr)), pFrame);
    }

    //Allocate a fresh image each frame so bits() never triggers a CoW detach
    //copying stale pixel data before we overwrite it.
    ScreenSize newSize = ScreenSize(desc.Height, desc.Width);
    pScreenSize = newSize;
    QImage writeFrame(newSize.width, newSize.height, QImage::Format_RGB32);
    BYTE* dst = writeFrame.bits();
    for (int h = 0; h < newSize.height; h++)
        memcpy(dst + h * newSize.bytesPerLine(), static_cast<const BYTE*>(sr.pData) + h * sr.RowPitch, newSize.bytesPerLine());

    pD3DDeviceContext->Unmap(pCpuTex, 0);

    pFrame = writeFrame;
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
        return Error("DXGIScreenCapture: CreateDevice failed with code %1").arg(HRESULT_CODE(hr));

    // Create GPU scaler now that we have a device and context
    pScaler = std::make_unique<D3D11Scaler>(pD3DDevice, pD3DDeviceContext, pScaleFactor);

    //Get DXGI device
    IDXGIDevice4* dxgiDevice = nullptr;
    hr = pD3DDevice->QueryInterface(__uuidof(IDXGIDevice4), reinterpret_cast<void**>(&dxgiDevice));
    if (FAILED(hr))
        return Error("DXGIScreenCapture: QueryInterface failed with code %1").arg(HRESULT_CODE(hr));

    //Get DXGI adapter
    IDXGIAdapter4* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter4), reinterpret_cast<void**>(&dxgiAdapter));
    dxgiDevice->Release();
    dxgiDevice = nullptr;
    if (FAILED(hr))
        return Error("DXGIScreenCapture: Could not get DXGI adapter - code %1").arg(HRESULT_CODE(hr));

    //Get output
    IDXGIOutput* dxgiOutput = nullptr;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    dxgiAdapter->Release();
    dxgiAdapter = nullptr;
    if (FAILED(hr))
        return Error("DXGIScreenCapture: Could not enumerate outputs - code %1").arg(HRESULT_CODE(hr));

    //Get DXGI output descriptor
    dxgiOutput->GetDesc(&pOutputDesc);

    //QueryInterface for Output 1
    IDXGIOutput6* dxgiOutput1 = nullptr;
    hr = dxgiOutput->QueryInterface(__uuidof(dxgiOutput1), reinterpret_cast<void**>(&dxgiOutput1));
    dxgiOutput->Release();
    dxgiOutput = nullptr;
    if (FAILED(hr))
        return Error("DXGIScreenCapture: QueryInterface1 failed with code %1").arg(HRESULT_CODE(hr));

    //Create desktop duplication
    const DXGI_FORMAT formats[] = { DXGI_FORMAT_B8G8R8A8_UNORM };
    try
    {
        //hr = dxgiOutput1->DuplicateOutput(pD3DDevice, &pDeskDupl);
        hr = dxgiOutput1->DuplicateOutput1(pD3DDevice, 0, ARRAYSIZE(formats), formats ,&pDeskDupl);
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
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
            //Too many desktop recorders active
            return Error("DXGIScreenCapture: Too many desktop recorders active");
        }
        return Error("DXGIScreenCapture: DuplicateOutput failed with code %1").arg(HRESULT_CODE(hr));
    }

    pInitialised = true;
    return NoError;
}

//Clean up variables
void DXGIScreenCapture::cleanup()
{
    pScaler.reset();
    if (pCpuTex)
        pCpuTex->Release();
    if (pDeskDupl)
        pDeskDupl->Release();
    if (pD3DDeviceContext)
        pD3DDeviceContext->Release();
    if (pD3DDevice)
        pD3DDevice->Release();

    pCpuTex = nullptr;
    pCpuTexWidth = 0;
    pCpuTexHeight = 0;
    pDeskDupl = nullptr;
    pD3DDeviceContext = nullptr;
    pD3DDevice = nullptr;
    pInitialised = false;
    pHaveFrameLock = false;
}

#endif
