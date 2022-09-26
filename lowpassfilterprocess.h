#pragma once

#include "imageprocess.h"
#include <QQueue>

//Low pass filter cutoff in Hertz
#define DEFAULT_FILTER_CUTOFF_HZ 1000

class LowPassFilterProcess : public ImageProcess
{
    /* Low pass filter to get rid of high frequency noise in given image. Stateful! */
public:
    LowPassFilterProcess(int filterCutoff = DEFAULT_FILTER_CUTOFF_HZ);

    ProcessType type() const {return ProcessType::LowPassFilter;}

    //Process the input image and return a filtered output
    QImage process(QImage inputImage);
private:
    QQueue<QImage> pPreviousImages; //TODO: Check that previous image sizes correspond to current state!
};
