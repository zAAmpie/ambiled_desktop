#pragma once

#include "screen.h"
class DXGIScreen;

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi1_2.h>

class DXGIScreen : public Screen
{
public:
    DXGIScreen();
    ~DXGIScreen();

    //Get methods
    CaptureMode type() const override {return Screen::DXGIMode;}

    //Main function to start capturing the screen
    QImage capture() override;
private:
    //Create variables (typically after screen change)
    void createVariables();

    bool pHaveFrameLock;
    QImage pFrame;

    //DXGI specific variables
    ID3D11Device* pD3DDevice;
    ID3D11DeviceContext* pD3DDeviceContext;
    IDXGIOutputDuplication* pDeskDupl;
    DXGI_OUTPUT_DESC pOutputDesc;
};

#endif
