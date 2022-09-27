#pragma once

#include "imageprocess.h"
#include <QElapsedTimer>
#include <QQueue>

//Low pass filter cutoff in Hertz
#define DEFAULT_FILTER_CUTOFF_HZ 10
#define DEFAULT_HISTORY_MAX 1

class LowPassFilterProcess : public ImageProcess
{
    /* Low pass filter to get rid of high frequency noise in given image. Stateful! */
public:
    LowPassFilterProcess(int filterCutoff = DEFAULT_FILTER_CUTOFF_HZ,
                         int historyMax = DEFAULT_HISTORY_MAX);
    ~LowPassFilterProcess();

    ProcessType type() const override {return ProcessType::LowPassFilter;}

    //Process the input image and return a filtered output
    QImage process(QImage inputImage) override;
private:
    //If newest image is different than previous ones, flush because calculations won't make sense
    void flushHistoryIfIncompatible(QImage &inputImage);

    void addToHistory(QImage &inputImage);

    std::unique_ptr<QElapsedTimer> pDeltaTimer;
    QQueue<QImage> pPreviousImages;
    int pFilterCutoff;
    int pHistoryMax;
};

