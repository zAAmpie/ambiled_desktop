#pragma once

#include <QImage>

struct ScreenSize
{
    /*Basic 2D struct for capturing screen size*/
    int height;
    int width;

    //Basic constructor
    ScreenSize(int h, int w) : height(h), width(w) {}

    //Zero values indicate an unset size
    bool isSet() {return height > 0 && width > 0;}
};

using ImageSize = ScreenSize; //Just an alias for existing struct of height x width

class Screen
{
    /*Base class for all screen capture types*/
public:
    enum CaptureMode {GDIMode, DirectX9Mode, DirectX11Mode, DXGIMode, X11Mode};

    Screen(const CaptureMode type, ScreenSize size);
    Screen(const CaptureMode);
    ~Screen();

    //Get/set methods
    CaptureMode type() const {return pType;}
    ScreenSize size() const {return pScreenSize;}
    void setSize(ScreenSize size) {pScreenSize = size;}

    //Main function to return the captured screen
    virtual QImage capture();
private:
    CaptureMode pType;
    ScreenSize pScreenSize;
};

