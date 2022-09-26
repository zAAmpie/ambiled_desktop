#pragma once

#include "imageprocess.h"
#include "imagetransform.h"
#include <QMap>
#include <QObject>

class ProcessManager : public QObject
{
    Q_OBJECT
    /* Manages all processing of a given screen image */
public:
    ProcessManager();
    ~ProcessManager();

    //Processing can be enabled and disabled (0 - n)
    void enableProcess(ImageProcess::ProcessType type);
    void disableProcess(ImageProcess::ProcessType type);

    //Set priority of process. Lower values = first execution
    void setProcessPriority(ImageProcess::ProcessType, int priority);

    //Only one transform can be used at a given point (1 - 1)
    void setTransform(ImageTransform::TransformType type);

    //Process the input screen and provide an LED output
    void startProcess(QImage screen);
signals:
    //Signals when processing has been completed
    void readyProcess(ImageStrips strips);
private:
    //Resorts the process list by priority
    void sortProcessList();

    QList<std::unique_ptr<ImageProcess>> pProcessList;
    QMap<ImageProcess::ProcessType, int> pProcessPriority;
    std::unique_ptr<ImageTransform> pTransform;
};

