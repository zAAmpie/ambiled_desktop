#pragma once

#include "imagetransform.h"

class SimpleTransform : public ImageTransform
{
    /* Simple transform that simply takes the 1px edge of the image to construct strips. Very fast transform. */
public:
    SimpleTransform();
    ~SimpleTransform();

    //Return type
    TransformType type() const override {return TransformType::SimpleTransform;}

    //Virtual function that transform an input image into strips
    ImageStrips transform(QImage inputImage, const StripSizes sizes) override;
private:
    //Averages an individual strip using block strip
    QImage averageStrip(const QImage &input, ImageSize size, StripPlacement placement);
};

