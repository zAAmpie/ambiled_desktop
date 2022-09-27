#pragma once

#include "screen.h"
class DX9Screen;

#ifdef Q_OS_WIN
#include <windef.h>
#include <d3d9.h>

class DX9Screen : public Screen
{
public:
    DX9Screen();
    ~DX9Screen();

    //Get methods
    CaptureMode type() const override {return Screen::DirectX9Mode;}

    //Main function to start capturing the screen
    QImage capture() override;
private:
    //Create variables (typically after screen change)
    void createVariables();

    HWND pDesktopWnd;

    //DirectX9 specific variables
    IDirect3D9 *pD3d;
    IDirect3DDevice9 *pDevice;
    IDirect3DSurface9 *pSurface;
};

#endif
