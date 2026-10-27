#pragma once

#include "imageprocess.h"
#include "imagetransform.h"
#include "perfstats.h"
#include <QMap>
#include <QObject>
#include <QMutex>
#include <map>

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

    //Is a process of this type currently enabled? - thread-safe
    bool isProcessEnabled(ImageProcess::ProcessType type) const;

    //Generic per-process configuration (see ImageProcess::ParameterSpec/parameters/setParameters) -
    //works for any process type without ProcessManager needing to know what the parameters mean.
    //Stored values persist across enable/disable, and apply live if the process is enabled - thread-safe
    QVariantMap processParameters(ImageProcess::ProcessType type) const;
    void setProcessParameters(ImageProcess::ProcessType type, const QVariantMap &parameters);

    //Metadata describing a process type's configurable parameters, for building generic config UI.
    //Callable without an instance, since a disabled process type has none.
    static QList<ImageProcess::ParameterSpec> parameterSpecsForType(ImageProcess::ProcessType type);

    //Only one transform can be used at a given point (1 - 1) - thread-safe
    void setTransform(ImageTransform::TransformType type);

    //Queue a screen for processing into an LED output - thread-safe, meant to be called directly from
    //the producer's thread. Only the newest frame is ever kept: if one is still waiting to be processed
    //it's replaced rather than queued behind, so a slow pipeline drops frames instead of adding latency.
    void startProcess(QImage screen, StripSizes sizes);
signals:
    //Signals when processing has been completed
    void readyProcess(ImageStrips strips);

    //Failure
    void failed(QString message);
private:
    //Runs on the process thread: processes whichever frame is pending
    void processPendingFrame();
    //Can the last output be reused as-is for this input (same frame, sizes and configuration)?
    bool canReuseLastStrips(const QImage &screen, const StripSizes &sizes) const;
    //Any configuration change means the last output can no longer be reused - call with pMutex held
    void invalidateLastStrips() {pLastFrameKey = 0;}

    //Resorts the process list by priority
    void sortProcessList();
    //Checks if process exists in the process list and returns the index
    int indexOfProcess(ImageProcess::ProcessType type);
    //Creates a process of a given type and passes through the pointer
    ImageProcess *createProcessOfType(ImageProcess::ProcessType type);

    QList<ImageProcess *> pProcessList;
    QMap<ImageProcess::ProcessType, ProcessStatus> pProcessStatus;
    QMap<ImageProcess::ProcessType, QVariantMap> pProcessParameters;
    std::unique_ptr<ImageTransform> pTransform;

    std::map<ImageProcess::ProcessType, PerfStats> pProcessStats;
    PerfStats pTransformStats;
    PerfStats pTotalStats;

    mutable QMutex pMutex;

    //Latest-frame-wins handoff from startProcess() to processPendingFrame()
    QMutex pPendingMutex;
    QImage pPendingFrame;
    StripSizes pPendingSizes;
    bool pProcessScheduled = false;

    //Last processed frame and its output, for reusing when the frame hasn't changed - guarded by pMutex
    qint64 pLastFrameKey = 0;
    StripSizes pLastSizes;
    ImageStrips pLastStrips;

    PerfStats pDroppedStats;
    PerfStats pReusedStats;
};

