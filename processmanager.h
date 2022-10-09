#pragma once

#include "imageprocess.h"
#include "imagetransform.h"
#include <QMap>
#include <QObject>
#include <QMutex>

struct ProcessStatus
{
    int priority;
    bool enabled;

    ProcessStatus(int prio, bool enable) : priority(prio), enabled(enable) {}
    ProcessStatus() : priority(0), enabled(false) {}
    bool operator<(ProcessStatus &rhs) {return priority < rhs.priority;}
};

class ProcessManager : public QObject
{
    Q_OBJECT
    /* Manages all processing of a given screen image */
public:
    ProcessManager(QObject *parent = nullptr);
    ~ProcessManager();

    //Processing can be enabled and disabled (0 - n) - thread-safe
    void setEnabledProcess(ImageProcess::ProcessType type, bool enabled);
    void enableProcess(ImageProcess::ProcessType type) {setEnabledProcess(type, true);}
    void disableProcess(ImageProcess::ProcessType type) {setEnabledProcess(type, false);}

    //Set priority of process. Lower values = first execution - thread-safe
    void setProcessPriority(ImageProcess::ProcessType type, int priority);

    //Only one transform can be used at a given point (1 - 1) - thread-safe
    void setTransform(ImageTransform::TransformType type);

    //Process the input screen and provide an LED output
    void startProcess(QImage screen, StripSizes sizes);
signals:
    //Signals when processing has been completed
    void readyProcess(ImageStrips strips);

    //Failure
    void failed(QString message);
private:
    //Resorts the process list by priority
    void sortProcessList();
    //Checks if process exists in the process list and returns the index
    int indexOfProcess(ImageProcess::ProcessType type);
    //Creates a process of a given type and passes through the pointer
    ImageProcess *createProcessOfType(ImageProcess::ProcessType type);

    QList<ImageProcess *> pProcessList;
    QMap<ImageProcess::ProcessType, ProcessStatus> pProcessStatus;
    std::unique_ptr<ImageTransform> pTransform;

    QMutex pMutex;
};

