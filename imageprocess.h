#pragma once

#include "types.h"

class QImage;

class ImageProcess
{
    /* Base class for all image process types*/
public:
    //Default order in list is the default priority, i.e. first will be executed first by default
    enum ProcessType {BlackBarRemoval, SampleScale, LinearScale, CubicScale, SimpleLowPassFilter};

    ImageProcess();
    ~ImageProcess();

    //Returns the type
    virtual ProcessType type() const;

    //Virtual function that processes an input image into an output
    virtual QImage process(QImage inputImage, ImageSize outputSize = ImageSize(0,0));
};

