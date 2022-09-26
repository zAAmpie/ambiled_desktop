#pragma once

#include "imagetransform.h"

class BlockStripTransform : public ImageTransform
{
    /* Transform that quickly scales to an intermediate resolution and then does the final smooth transform to strips. Not as expensive as BlockTransform. */
public:
    BlockStripTransform();
    ~BlockStripTransform();

    //Return type
    TransformType type() const {return TransformType::BlockStripTransform;}

    //Virtual function that transform an input image into strips
    ImageStrips transform(QImage inputImage, const StripSizes sizes);
};


