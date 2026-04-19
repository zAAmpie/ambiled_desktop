#pragma once

#include "screencapture.h"
#include "d3d11scaler.h"
#include <memory>
class DXGIScreenCapture;

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi1_6.h>

class DXGIScreenCapture : public ScreenCapture
{
public:
    DXGIScreenCapture();
    ~DXGIScreenCapture();

    //Get methods
    CaptureMode type() const override {return ScreenCapture::DXGIMode;}

    //Main function to start capturing the screen
    CaptureValue capture() override;

    // GPU downscale factor applied before CPU readback (0 < factor <= 1.0)
    void setGPUScaleFactor(float factor) override;
    float scaleFactor() const;

private:
    //Create variables (typically after screen change)
    Error createVariables();

    //Clean up variables
    void cleanup();

    bool pHaveFrameLock;

    //DXGI specific variables
    ID3D11Device* pD3DDevice;
    ID3D11DeviceContext* pD3DDeviceContext;
    IDXGIOutputDuplication* pDeskDupl;
    DXGI_OUTPUT_DESC pOutputDesc;

    // Staging texture reused across frames; recreated only on resolution change
    ID3D11Texture2D* pCpuTex;
    UINT pCpuTexWidth;
    UINT pCpuTexHeight;

    // GPU-side bilinear downscaler; created after device initialisation
    std::unique_ptr<D3D11Scaler> pScaler;
    float pScaleFactor;
};

#endif
