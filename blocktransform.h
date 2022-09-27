#pragma once

#include "imagetransform.h"

#define DEFAULT_STRIP_WIDTH_PERCENTAGE 0.1

class BlockTransform : public ImageTransform
{
    /* Transform that scales smoothly from the input image to strips. Tends to be expensive */
public:
    BlockTransform(float stripWidthPercentage = DEFAULT_STRIP_WIDTH_PERCENTAGE);
    ~BlockTransform();

    //Return type
    TransformType type() const override {return TransformType::BlockTransform;}

    //Virtual function that transform an input image into strips
    ImageStrips transform(QImage inputImage, const StripSizes sizes) override;
private:
    //Averages an individual strip using block transform
    QImage averageStrip(const QImage &input, ImageSize size, StripPlacement placement);

    float pStripWidthPercentage;
};


