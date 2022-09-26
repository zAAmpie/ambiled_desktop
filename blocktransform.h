#pragma once

#include "imagetransform.h"

class BlockTransform : public ImageTransform
{
    /* Transform that scales smoothly from the input image to strips. Tends to be expensive */
public:
    BlockTransform();
    ~BlockTransform();

    //Return type
    TransformType type() const {return TransformType::BlockTransform;}

    //Virtual function that transform an input image into strips
    ImageStrips transform(QImage inputImage, const StripSizes sizes);
};


