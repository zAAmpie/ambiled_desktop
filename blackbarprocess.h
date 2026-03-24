#pragma once

#include "imageprocess.h"

class BlackBarProcess : public ImageProcess
{
    /* Removes black bars from widescreen content */
public:
    static constexpr float DEFAULT_THRESHOLD_PERCENTAGE = 0.25f;
    static constexpr float DEFAULT_BLACK_VALUE_THRESHOLD = 5.0f;
    // Sample every Nth column per row — reduces inner-loop work ~8x while
    // still reliably detecting any non-black content in letterbox bars
    static constexpr int COLUMN_SAMPLE_STEP = 8;

    BlackBarProcess(float scanThresholdPercentage = DEFAULT_THRESHOLD_PERCENTAGE,
                    float blackValueThreshold = DEFAULT_BLACK_VALUE_THRESHOLD);
    ~BlackBarProcess();

    ProcessType type() const override {return ProcessType::BlackBarRemoval;}

    //Process the image and remove black bars
    QImage process(const QImage& inputImage) override;
private:
    int getFirstNonBlackLine(const QImage& image);
    int getLastNonBlackLine(const QImage& image);

    float pScanThreshold;
    float pValueThreshold;
};


