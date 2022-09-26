#pragma once

#include "imageprocess.h"
#include "types.h"

class ScaleProcess : public ImageProcess
{
    /* Scales an image for further processing using a variety of interpolation methods */
public:
    ScaleProcess(ImageProcess::ProcessType type, ImageSize outputSize);

    ProcessType type() const;

    //Scales image using the selected scaler type
    QImage process(QImage inputImage);
private:
    ProcessType pType;
};

