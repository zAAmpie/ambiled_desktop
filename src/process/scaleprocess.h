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

    //Generic configuration contract (see ImageProcess) - "scalePercentage" (10-90, integer).
    //Interpolation mode is not a parameter here - it's a choice between which ProcessType
    //is enabled (SampleScale/LinearScale/CubicScale), handled by ProcessManager/the UI.
    static QList<ImageProcess::ParameterSpec> parameterSpecs();
    QVariantMap parameters() const override;
    void setParameters(const QVariantMap &parameters) override;
private:
    ProcessType pType;
    ImageSize pOutputSize;
    float pScalePercentage;

#ifdef Q_OS_WIN
    IWICImagingFactory *pWICFactory;
    IWICBitmapScaler *pWICScaler;
#endif
};

