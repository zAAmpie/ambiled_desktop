#pragma once

#include "imagetransform.h"

#define DEFAULT_STRIP_WIDTH_PERCENTAGE 0.1

class BlockStripTransform : public ImageTransform
{
    /* Transform that quickly scales to an intermediate resolution and then does the final smooth transform to strips. Not as expensive as BlockTransform. */
public:
    BlockStripTransform(float stripWidthPercentage = DEFAULT_STRIP_WIDTH_PERCENTAGE);
    ~BlockStripTransform();

    //Return type
    TransformType type() const override {return TransformType::BlockStripTransform;}

    //Virtual function that transform an input image into strips
    ImageStrips transform(QImage inputImage, const StripSizes sizes) override;
private:
    //Averages an individual strip using block strip
    QImage averageStrip(const QImage &input, ImageSize size, StripPlacement placement);

    float pStripWidthPercentage;
};


