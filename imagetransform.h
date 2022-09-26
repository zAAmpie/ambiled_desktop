#pragma once

#include "leds.h"
#include "types.h"

struct Offset
{
    int x; //x offset from top left
    int y; //y offset from top left
};

class ImageTransform
{
    /* Base class for all objects that transform from an image to ImageStrips */
public:
    enum TransformType {SimpleTransform, StripTransform, BlockStripTransform, BlockTransform};
    enum StripPlacement {TopStrip, BottomStrip, LeftStrip, RightStrip};

    ImageTransform();
    ~ImageTransform();

    //Return type
    virtual TransformType type() const;

    //Virtual function that transform an input image into strips
    virtual ImageStrips transform(QImage inputImage, const StripSizes sizes);
private:
    //Copy a specified strip width of an image from a specified side
    QImage copyStrip(const QImage input, int stripSizePx, StripPlacement placement);
};

