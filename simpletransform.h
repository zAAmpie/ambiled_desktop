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
private:
    //Averages an individual strip using block strip
    QImage averageStrip(const QImage &input, ImageSize size, StripPlacement placement) override;
};

