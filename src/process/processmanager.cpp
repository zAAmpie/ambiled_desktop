#include "processmanager.h"
#include "blackbarprocess.h"
#include "scaleprocess.h"
#include "lowpassfilterprocess.h"
#include "blockstriptransform.h"
#include "blocktransform.h"
#include "simpletransform.h"
#include "striptransform.h"
#include <QMetaEnum>

ProcessManager::ProcessManager(QObject *parent) : QObject(parent)
{
    //Create default priorities - first in the enum gets executed first
    const QMetaEnum metaEnum = QMetaEnum::fromType<ImageProcess::ProcessType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i)
        pProcessStatus.insert(static_cast<ImageProcess::ProcessType>(metaEnum.value(i)), ProcessStatus(i, false));

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
    //Run through all processing steps in priority order
    QImage tempImage = screen;
    for (int i = 0; i < pProcessList.size(); ++i)
    {
        tempImage = pProcessList.at(i)->process(tempImage);
    }

    //Transform to strips
    ImageStrips outputStrips = pTransform->transform(tempImage, sizes);

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
    switch (type)
    {
    case ImageProcess::BlackBarRemoval:
        return new BlackBarProcess();
    case ImageProcess::LowPassFilter:
        return new LowPassFilterProcess();
    //TODO: Make these configurable, currently hardcoded to 50% scaling
    case ImageProcess::CubicScale:
        return new ScaleProcess(ImageProcess::CubicScale, 0.5);
    case ImageProcess::LinearScale:
        return new ScaleProcess(ImageProcess::LinearScale, 0.5);
    case ImageProcess::SampleScale:
        return new ScaleProcess(ImageProcess::SampleScale, 0.5);
    default:
        return nullptr;
    }
}

;
