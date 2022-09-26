#pragma once

#include "leds.h"
#include "types.h"

class ImageTransform
{
    /* Base class for all objects that transform from an image to ImageStrips */
public:
    enum TransformType {SimpleTransform, StripTransform, BlockStripTransform, BlockTransform};

    ImageTransform();
    ~ImageTransform();

    //Return type
    virtual TransformType type() const;

    //Virtual function that transform an input image into strips
    virtual ImageStrips transform(QImage inputImage, const StripSizes sizes);
};

