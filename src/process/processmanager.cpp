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
    : QObject(parent), pTransformStats(QStringLiteral("transform")), pTotalStats(QStringLiteral("process.total")),
      pDroppedStats(QStringLiteral("process.dropped")), pReusedStats(QStringLiteral("process.reused"))
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
    invalidateLastStrips();

    pMutex.unlock();
}


//Set priority of process. Lower values = first execution - thread-safe
void ProcessManager::setProcessPriority(ImageProcess::ProcessType type, int priority)
{
    pMutex.lock();
    pProcessStatus[type].priority = priority;

    //Re-sort list now that priority has changed
    sortProcessList();
    invalidateLastStrips();
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
    invalidateLastStrips();
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
    invalidateLastStrips();
    pMutex.unlock();
}

//Queue a screen for processing - see header. Runs on the caller's thread.
void ProcessManager::startProcess(QImage screen, StripSizes sizes)
{
    QMutexLocker locker(&pPendingMutex);
    if (!pPendingFrame.isNull())
        pDroppedStats.addEvent(); //Previous frame never got processed - replaced below

    pPendingFrame = std::move(screen);
    pPendingSizes = sizes;

    //Only ever one processing call in flight on the process thread's event queue
    if (!pProcessScheduled)
    {
        pProcessScheduled = true;
        QMetaObject::invokeMethod(this, &ProcessManager::processPendingFrame, Qt::QueuedConnection);
    }
}

//Runs on the process thread: process the input screen and provide an LED output
void ProcessManager::processPendingFrame()
{
    QImage screen;
    StripSizes sizes;
    {
        QMutexLocker locker(&pPendingMutex);
        screen = std::move(pPendingFrame);
        pPendingFrame = QImage();
        sizes = pPendingSizes;
        pProcessScheduled = false;
    }
    if (screen.isNull())
        return;

    //Held throughout, so the GUI thread can't change the process list/transform mid-frame
    QMutexLocker locker(&pMutex);

    if (canReuseLastStrips(screen, sizes))
    {
        //Still emit, so downstream (LEDs, serial) behaves exactly as if the frame had been processed
        pReusedStats.addEvent();
        emit readyProcess(pLastStrips);
        return;
    }

    ImageStrips outputStrips;
    {
        PerfTimer totalTimer(pTotalStats);

        //Run through all before-transform processing steps in priority order
        QImage tempImage = screen;
        for (ImageProcess *process : std::as_const(pProcessList))
        {
            if (process->stage() != ImageProcess::BeforeTransform)
                continue;
            PerfTimer timer(pProcessStats.at(process->type()));
            tempImage = process->process(tempImage);
        }

        //Transform to strips
        {
            PerfTimer transformTimer(pTransformStats);
            outputStrips = pTransform->transform(tempImage, sizes);
        }

        //Then the after-transform steps, on all strips packed into one image (only packed if needed)
        QImage packedStrips;
        for (ImageProcess *process : std::as_const(pProcessList))
        {
            if (process->stage() != ImageProcess::AfterTransform)
                continue;
            if (packedStrips.isNull())
                packedStrips = packStrips(outputStrips);
            PerfTimer timer(pProcessStats.at(process->type()));
            packedStrips = process->process(packedStrips);
        }
        if (!packedStrips.isNull())
            unpackStrips(packedStrips, outputStrips);
    }

    pLastFrameKey = screen.cacheKey();
    pLastSizes = sizes;
    pLastStrips = outputStrips;

    //Signals when processing has been completed
    emit readyProcess(outputStrips);
}

//Packs all four strips end to end (top, bottom, left, right) into one 1-row RGB32 image
QImage ProcessManager::packStrips(const ImageStrips &strips)
{
    const QImage parts[] = {strips.topLine.convertToFormat(QImage::Format_RGB32), strips.bottomLine.convertToFormat(QImage::Format_RGB32),
                            strips.leftLine.convertToFormat(QImage::Format_RGB32), strips.rightLine.convertToFormat(QImage::Format_RGB32)};

    int totalPixels = 0;
    for (const QImage &part : parts)
        totalPixels += part.width() * part.height();

    QImage packed(totalPixels, 1, QImage::Format_RGB32);
    QRgb *out = reinterpret_cast<QRgb *>(packed.scanLine(0));
    for (const QImage &part : parts)
        for (int y = 0; y < part.height(); ++y)
            out = std::copy_n(reinterpret_cast<const QRgb *>(part.constScanLine(y)), part.width(), out);

    return packed;
}

//Inverse of packStrips - splits a packed image back into strips of the sizes already in `strips`
void ProcessManager::unpackStrips(const QImage &packed, ImageStrips &strips)
{
    const QRgb *in = reinterpret_cast<const QRgb *>(packed.constScanLine(0));
    for (QImage *part : {&strips.topLine, &strips.bottomLine, &strips.leftLine, &strips.rightLine})
    {
        QImage unpacked(part->size(), QImage::Format_RGB32);
        for (int y = 0; y < unpacked.height(); ++y)
        {
            std::copy_n(in, unpacked.width(), reinterpret_cast<QRgb *>(unpacked.scanLine(y)));
            in += unpacked.width();
        }
        *part = unpacked;
    }
}

//Can the last output be reused as-is for this input? - call with pMutex held
bool ProcessManager::canReuseLastStrips(const QImage &screen, const StripSizes &sizes) const
{
    if (pLastFrameKey == 0 || screen.cacheKey() != pLastFrameKey || sizes != pLastSizes)
        return false;

    //A stateful process (e.g. the low-pass filter) still needs every tick to converge
    for (const ImageProcess *process : pProcessList)
        if (process->isStateful())
            return false;
    return true;
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
