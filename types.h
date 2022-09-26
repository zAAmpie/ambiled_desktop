#pragma once

typedef unsigned char BYTE;

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
using ImageSize = ScreenSize;
