#pragma once

#include "imageprocess.h"
#include <QElapsedTimer>
#include <QVector>

//Low pass filter cutoff in Hertz
#define DEFAULT_FILTER_CUTOFF_HZ 10

class LowPassFilterProcess : public ImageProcess
{
    /* First-order low pass filter (exponential smoothing) over time, to get rid of high frequency
     * flicker. Runs after the transform, i.e. on the LED values only. Stateful! */
public:
    LowPassFilterProcess(int filterCutoff = DEFAULT_FILTER_CUTOFF_HZ);
    ~LowPassFilterProcess();

    ProcessType type() const override {return ProcessType::LowPassFilter;}
    Stage stage() const override {return AfterTransform;}
    bool isStateful() const override {return true;}

    //Process the input image and return a filtered output
    QImage process(QImage inputImage) override;

    //Generic configuration contract (see ImageProcess) - "filterCutoffHz"
    static QList<ImageProcess::ParameterSpec> parameterSpecs();
    QVariantMap parameters() const override;
    void setParameters(const QVariantMap &parameters) override;
private:
    QElapsedTimer *pDeltaTimer;
    int pFilterCutoff;

    //Filtered value of every byte of the image, kept in float so slow fades don't stall on rounding.
    //Reset whenever the input's layout changes, since the values would no longer line up.
    QVector<float> pState;
    QSize pStateSize;
    QImage::Format pStateFormat;
};

