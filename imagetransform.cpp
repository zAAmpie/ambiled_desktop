#include "imagetransform.h"

//Constructor
ImageTransform::ImageTransform()
{

}

//Destructor
ImageTransform::~ImageTransform()
{

}

//Function that transform an input image into strips
ImageStrips ImageTransform::transform(QImage inputImage, const StripSizes sizes)
{
    Q_ASSERT(!inputImage.isNull());
    Q_ASSERT(sizes.isSet());

    ImageStrips outStrips;

    //Do each of the strips
    outStrips.topLine = averageStrip(inputImage, ImageSize(1, sizes.top), StripPlacement::TopStrip);
    outStrips.bottomLine = averageStrip(inputImage, ImageSize(1, sizes.bottom), StripPlacement::BottomStrip);
    outStrips.leftLine = averageStrip(inputImage, ImageSize(sizes.left, 1), StripPlacement::LeftStrip);
    outStrips.rightLine = averageStrip(inputImage, ImageSize(sizes.right, 1), StripPlacement::RightStrip);

    return outStrips;
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

