#pragma once

#include <QPair>
#include <QString>

typedef unsigned char BYTE;

//General return value of an error message with return type of void
using Error = QString;
#define NoError "" //TODO: Check if we should rather redefine as Q_GLOBAL_STATIC

//General return value of an error message with associated return type T
template <typename T>
struct ReturnValue
{
    Error error;
    T value;
    //Basic constructors
    ReturnValue(Error e, T val) : error(e), value(val){}
    ReturnValue(T val) : error(NoError), value(val){} //short-hand for successful return values
    bool success() const {return error == NoError;}
    bool failed() const {return !success();}
};

struct ScreenSize
{
    /*Basic 2D struct for capturing screen size*/
    int height;
    int width;
    int depth; //in bytes

    //Basic constructor
    ScreenSize(int h, int w, int d = 4) : height(h), width(w), depth(d) {}
    ScreenSize() : height(0), width(0), depth(4) {}

    //Zero values indicate an unset size
    bool isSet() const {return height > 0 || width > 0;}

    //Get sizes of a screen in bytes
    long long totalBytes() const {return (long long)height * width * depth;}
    long long bytesPerLine() const {return (long long)width * depth;}

    bool operator==(ScreenSize &rhs) {return height == rhs.height && width == rhs.width && depth == rhs.depth;}
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



