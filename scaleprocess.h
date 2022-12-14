#pragma once

#include "imageprocess.h"
#include "types.h"

#ifdef Q_OS_WIN
#include <wincodec.h>
#endif

class ScaleProcess : public ImageProcess
{
    /* Scales an image for further processing using a variety of interpolation methods */
public:
    ScaleProcess(ImageProcess::ProcessType type, ImageSize outputSize);
    ScaleProcess(ImageProcess::ProcessType type, float scalePercentage);
    ~ScaleProcess();

    ProcessType type() const override {return pType;}

    //Scales image using the selected scaler type
    QImage process(QImage inputImage) override;
private:
    ProcessType pType;
    ImageSize pOutputSize;
    float pScalePercentage;

#ifdef Q_OS_WIN
    IWICImagingFactory *pWICFactory;
    IWICBitmapScaler *pWICScaler;
#endif
};

