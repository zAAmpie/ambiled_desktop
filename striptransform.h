#pragma once

#include "imagetransform.h"

#define DEFAULT_STRIP_WIDTH_PERCENTAGE 0.1

class StripTransform : public ImageTransform
{
    /* Transform that fast averages into intermediate strips and then smoothly transforms to final strips. Faster than BlocksTripTransform. */
public:
    StripTransform(float stripWidthPercentage = DEFAULT_STRIP_WIDTH_PERCENTAGE);
    ~StripTransform();

    //Return type
    TransformType type() const override {return TransformType::StripTransform;}

    //Virtual function that transform an input image into strips
    ImageStrips transform(QImage inputImage, const StripSizes sizes) override;
private:
    //Averages an individual strip using block strip
    QImage averageStrip(const QImage &input, ImageSize size, StripPlacement placement);

    float pStripWidthPercentage;
};

