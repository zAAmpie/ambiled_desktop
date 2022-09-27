#pragma once

#include "screen.h"
class DX11Screen;

#ifdef Q_OS_WIN
#include <windef.h>
#include <d3d11.h>

class DX11Screen : public Screen
{
public:
    DX11Screen();
    ~DX11Screen();

    //Get methods
    CaptureMode type() const override {return Screen::DirectX11Mode;}

    //Main function to start capturing the screen
    QImage capture() override;
private:
    //Calculate window size
    ScreenSize getWindowSize(HWND &window);

    //Create variables (typically after screen change)
    void createVariables();
    //Create the swap chain
    void createSwapChain();
    //Create the texture
    void createTexture();

    HWND pDesktopWnd;
    ScreenSize pScreenSize;

    //DirectX11 specific variables
    ID3D11Device *pDevice11;
    ID3D11DeviceContext *pDeviceContext;
    IDXGISwapChain *pSwapChain;
    DXGI_SWAP_CHAIN_DESC pScDesc;
    ID3D11Texture2D *pBackBuffer;
};

#endif
