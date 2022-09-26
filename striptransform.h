#pragma once

#include "imagetransform.h"

class StripTransform : public ImageTransform
{
    /* Transform that fast averages into intermediate strips and then smoothly transforms to final strips. Faster than BlocksTripTransform. */
public:
    StripTransform();
    ~StripTransform();

    //Return type
    TransformType type() const {return TransformType::StripTransform;}

    //Virtual function that transform an input image into strips
    ImageStrips transform(QImage inputImage, const StripSizes sizes);
};

