#include "processmanager.h"
#include "blackbarprocess.h"
#include "scaleprocess.h"
#include "lowpassfilterprocess.h"
#include "blockstriptransform.h"
#include "blocktransform.h"
#include "simpletransform.h"
#include "striptransform.h"
#include <QMetaEnum>

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent), pTransformStats(QStringLiteral("transform")), pTotalStats(QStringLiteral("process.total"))
{
    //Create default priorities - first in the enum gets executed first
    const QMetaEnum metaEnum = QMetaEnum::fromType<ImageProcess::ProcessType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i)
    {
        ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(metaEnum.value(i));
        pProcessStatus.insert(type, ProcessStatus(i, false));
        pProcessStats.try_emplace(type, QStringLiteral("process.%1").arg(QLatin1String(metaEnum.key(i))));

        //Seed default parameter values generically from a throwaway instance, rather than
        //duplicating each process's own defaults here
        std::unique_ptr<ImageProcess> defaultInstance(createProcessOfType(type));
        if (defaultInstance)
            pProcessParameters.insert(type, defaultInstance->parameters());
    }

    //Create default transform
    pTransform = std::make_unique<BlockStripTransform>();
}

ProcessManager::~ProcessManager()
{

}

//Processing can be enabled and disabled (0 - n) - thread-safe
void ProcessManager::setEnabledProcess(ImageProcess::ProcessType type, bool enabled)
{
    pMutex.lock();
    pProcessStatus[type].enabled = enabled;

    //Add to the process list
    int index = indexOfProcess(type);
    if (enabled && index == -1)
    {
        pProcessList.append(createProcessOfType(type));
        //Re-sort list now that new processes have been added
        sortProcessList();
    }
    //Remove from the process list
    else if (!enabled && index != -1)
    {
        pProcessList.removeAt(index);
    }

    pMutex.unlock();
}


//Set priority of process. Lower values = first execution - thread-safe
void ProcessManager::setProcessPriority(ImageProcess::ProcessType type, int priority)
{
    pMutex.lock();
    pProcessStatus[type].priority = priority;

    //Re-sort list now that priority has changed
    sortProcessList();
    pMutex.unlock();
}

//Is a process of this type currently enabled? - thread-safe
bool ProcessManager::isProcessEnabled(ImageProcess::ProcessType type) const
{
    QMutexLocker locker(&pMutex);
    return pProcessStatus.value(type).enabled;
}

//Current parameter values for a process type - thread-safe
QVariantMap ProcessManager::processParameters(ImageProcess::ProcessType type) const
{
    QMutexLocker locker(&pMutex);
    return pProcessParameters.value(type);
}

//Update parameter values for a process type, applying live if it's currently enabled - thread-safe
void ProcessManager::setProcessParameters(ImageProcess::ProcessType type, const QVariantMap &parameters)
{
    pMutex.lock();
    pProcessParameters[type] = parameters;

    int index = indexOfProcess(type);
    if (index != -1)
        pProcessList.at(index)->setParameters(parameters);
    pMutex.unlock();
}

//Metadata describing a process type's configurable parameters, for building generic config UI
QList<ImageProcess::ParameterSpec> ProcessManager::parameterSpecsForType(ImageProcess::ProcessType type)
{
    switch (type)
    {
    case ImageProcess::BlackBarRemoval:
        return BlackBarProcess::parameterSpecs();
    case ImageProcess::LowPassFilter:
        return LowPassFilterProcess::parameterSpecs();
    case ImageProcess::SampleScale:
    case ImageProcess::LinearScale:
    case ImageProcess::CubicScale:
        return ScaleProcess::parameterSpecs();
    default:
        return QList<ImageProcess::ParameterSpec>();
    }
}

//Only one transform can be used at a given point (1 - 1) - thread-safe
void ProcessManager::setTransform(ImageTransform::TransformType type)
{
    if (type == pTransform->type())
        //Already this type
        return;

    pMutex.lock();
    //Create a new transform
    switch (type)
    {
    case ImageTransform::BlockStripTransform:
        pTransform = std::make_unique<BlockStripTransform>();
        break;
    case ImageTransform::StripTransform:
        pTransform = std::make_unique<StripTransform>();
        break;
    case ImageTransform::SimpleTransform:
        pTransform = std::make_unique<SimpleTransform>();
        break;
    case ImageTransform::BlockTransform:
        pTransform = std::make_unique<BlockTransform>();
        break;
    }
    pMutex.unlock();
}

//Process the input screen and provide an LED output
void ProcessManager::startProcess(QImage screen, StripSizes sizes)
{
    ImageStrips outputStrips;
    {
        PerfTimer totalTimer(pTotalStats);

        //Run through all processing steps in priority order
        QImage tempImage = screen;
        for (int i = 0; i < pProcessList.size(); ++i)
        {
            ImageProcess *process = pProcessList.at(i);
            PerfTimer timer(pProcessStats.at(process->type()));
            tempImage = process->process(tempImage);
        }

        //Transform to strips
        PerfTimer transformTimer(pTransformStats);
        outputStrips = pTransform->transform(tempImage, sizes);
    }

    //Signals when processing has been completed
    emit readyProcess(outputStrips);
}


//Resorts the process list by priority
void ProcessManager::sortProcessList()
{
    //Sort by priority (get priority from map)
    std::sort(pProcessList.begin(), pProcessList.end(), [=] (ImageProcess *v1, ImageProcess *v2) {return pProcessStatus.value(v1->type()).priority < pProcessStatus.value(v2->type()).priority;});
}

//Checks if process exists in the process list and returns the index
int ProcessManager::indexOfProcess(ImageProcess::ProcessType type)
{
    for (int i = 0; i < pProcessList.size(); ++i)
    {
        if (pProcessList.at(i)->type() == type)
            return i;
    }
    return -1;
}

//Creates a process of a given type and passes through the pointer
ImageProcess *ProcessManager::createProcessOfType(ImageProcess::ProcessType type)
{
    ImageProcess *process = nullptr;
    switch (type)
    {
    case ImageProcess::BlackBarRemoval:
        process = new BlackBarProcess();
        break;
    case ImageProcess::LowPassFilter:
        process = new LowPassFilterProcess();
        break;
    case ImageProcess::CubicScale:
        process = new ScaleProcess(ImageProcess::CubicScale, 0.5f);
        break;
    case ImageProcess::LinearScale:
        process = new ScaleProcess(ImageProcess::LinearScale, 0.5f);
        break;
    case ImageProcess::SampleScale:
        process = new ScaleProcess(ImageProcess::SampleScale, 0.5f);
        break;
    default:
        return nullptr;
    }

    //Apply any previously-configured parameters generically (no-op the first time a type is
    //ever created, since pProcessParameters isn't seeded for it yet)
    if (pProcessParameters.contains(type))
        process->setParameters(pProcessParameters.value(type));

    return process;
}

;
