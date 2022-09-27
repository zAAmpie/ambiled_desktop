#pragma once

#include "screencapture.h"
class DX11ScreenCapture;

#ifdef Q_OS_WIN
#include <windef.h>
#include <d3d11.h>

class DX11ScreenCapture : public ScreenCapture
{
public:
    DX11ScreenCapture();
    ~DX11ScreenCapture();

    //Get methods
    CaptureMode type() const override {return ScreenCapture::DirectX11Mode;}

    //Main function to start capturing the screen
    QImage capture() override;
private:
    //Create variables (typically after screen change)
    void createVariables();
    //Create the swap chain
    void createSwapChain();
    //Create the texture
    void createTexture();

    HWND pDesktopWnd;

    //DirectX11 specific variables
    ID3D11Device *pDevice11;
    ID3D11DeviceContext *pDeviceContext;
    IDXGISwapChain *pSwapChain;
    DXGI_SWAP_CHAIN_DESC pScDesc;
    ID3D11Texture2D *pBackBuffer;
};

#endif
