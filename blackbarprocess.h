#pragma once

#include "imageprocess.h"

//Maximum percentage of the screen to search before giving up, e.g. 0.25 = 25% of the top/bottom
#define DEFAULT_THRESHOLD_PERCENTAGE 0.25
//Minimum value to consider "black". Can use a non-zero value to account for noise in poor quality video
#define DEFAULT_BLACK_VALUE_THRESHOLD 5

class BlackBarProcess : public ImageProcess
{
    /* Removes black bars from widescreen content */
public:
    BlackBarProcess(float scanThresholdPercentage = DEFAULT_THRESHOLD_PERCENTAGE,
                    float blackValueThreshold = DEFAULT_BLACK_VALUE_THRESHOLD);
    ~BlackBarProcess();

    ProcessType type() const {return ProcessType::BlackBarRemoval;}

    //Process the image and remove black bars
    QImage process(QImage inputImage);
private:
    //Test to see if line is black (within a theshold)
    int getFirstNonBlackLine(const QImage image); //TODO: See if we can speed this up
    int getLastNonBlackLine(const QImage image);


    float pScanThreshold;
    float pValueThreshold;
};


