#ifndef DX11SCREEN_H
#define DX11SCREEN_H

#include "screen.h"

class DX11Screen : public Screen
{
public:
    DX11Screen(const CaptureMode type, ScreenSize size);
    DX11Screen(const CaptureMode);
    ~DX11Screen();

    //Main function to return the captured screen
    QImage capture();
private:
    QImage pBuffer;
};

#endif // DX11SCREEN_H
