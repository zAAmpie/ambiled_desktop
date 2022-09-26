#pragma once

#include "imageprocess.h"
#include "imagetransform.h"
#include <QMap>

class ProcessManager
{
    /* Manages all processing of a given screen image */
public:
    ProcessManager();

    //Processing can be enabled and disabled (0 - n)
    void enableProcess(ImageProcess::ProcessType type);
    void disableProcess(ImageProcess::ProcessType type);

    //Set priority of process. Lower values = first execution
    void setProcessPriority(ImageProcess::ProcessType, int priority);

    //Only one transform can be used at a given point (1 - 1)
    void setTransform(ImageTransform::TransformType type);

    //Process the input screen and provide an LED output
    ImageStripsInput process(QImage screen);
private:
    QList<ImageProcess> pProcessList;
    QMap<ImageProcess::ProcessType, int> pProcessPriority;
    ImageTransform pTransform;
};

