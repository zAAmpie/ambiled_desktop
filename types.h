#pragma once

typedef unsigned char BYTE;

struct ScreenSize
{
    /*Basic 2D struct for capturing screen size*/
    int height;
    int width;

    //Basic constructor
    ScreenSize(int h, int w) : height(h), width(w) {}
    ScreenSize() : height(0), width(0) {}

    //Zero values indicate an unset size
    bool isSet() const {return height > 0 || width > 0;}

    bool operator==(ScreenSize &rhs) {return height == rhs.height && width == rhs.width;}
    bool operator!=(ScreenSize &rhs) {return !operator==(rhs);}
};
using ImageSize = ScreenSize;


struct StripSizes
{
    /*Basic 4D struct for capturing strip lengths*/
    int top;
    int bottom;
    int left;
    int right;

    //Basic constructor
    StripSizes(int t, int b, int l, int r) : top(t), bottom(b), left(l), right(r) {}
    StripSizes() : top(0), bottom(0), left(0), right(0) {}

    //Zero values indicate unset
    bool isSet() const {return top > 0 || bottom > 0 || left > 0 || right > 0;}
};
