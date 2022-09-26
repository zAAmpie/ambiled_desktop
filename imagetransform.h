#pragma once

#include "leds.h"

class ImageTransform
{
    /* Base class for all objects that transform from an image to ImageStrips */
public:
    enum TransformType {SimpleAveraging, StripAveraging, BlockStripAveraging, BlockAveraging};

    ImageTransform();

    //Return type
    virtual TransformType type() const;

    //Virtual function that transform an input image into strips
    virtual ImageStrips transform(QImage inputImage, const LEDPositions &positions);
};

