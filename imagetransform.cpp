#include "imagetransform.h"

//Constructor
ImageTransform::ImageTransform()
{

}

//Destructor
ImageTransform::~ImageTransform()
{

}

//Copy a specified strip width of an image from a specified side
QImage ImageTransform::copyStrip(const QImage input, int stripSizePx, StripPlacement placement)
{
    switch (placement)
    {
    case TopStrip:
        return input.copy(0, 0, input.width(), stripSizePx);
    case BottomStrip:
        return input.copy(0, input.height() - stripSizePx - 1, input.width(), stripSizePx);
    case LeftStrip:
        return input.copy(0, 0, stripSizePx, input.height());
    case RightStrip:
        return input.copy(input.width() - stripSizePx - 1, 0, stripSizePx, input.height());
    default:
        return QImage();
    }
}
