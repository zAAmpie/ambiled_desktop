#pragma once

#include "screencapture.h"
class DX9ScreenCapture;

#ifdef Q_OS_WIN
#include <windef.h>
#include <d3d9.h>

class DX9ScreenCapture : public ScreenCapture
{
public:
    DX9ScreenCapture();
    ~DX9ScreenCapture();

    //Get methods
    CaptureMode type() const override {return ScreenCapture::DirectX9Mode;}

    //Main function to start capturing the screen
    CaptureValue capture() override;
private:
    //Create variables (typically after screen change)
    Error createVariables();

    //Clean up variables
    void cleanup();

    HWND pDesktopWnd;

    //DirectX9 specific variables
    IDirect3D9 *pD3d;
    IDirect3DDevice9 *pDevice;
    IDirect3DSurface9 *pSurface;
};

#endif
