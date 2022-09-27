#include "lowpassfilterprocess.h"
#include "qmath.h"
#include "types.h"
#include "util.h"

//Constructor
LowPassFilterProcess::LowPassFilterProcess(int filterCutoff, int historyMax) : ImageProcess()
{
    Q_ASSERT(filterCutoff > 0 && filterCutoff <= 20000); //Warn at >20kHz, unlikely to make any difference
    Q_ASSERT(historyMax > 0);
    pFilterCutoff = qMax(0, filterCutoff);
    pHistoryMax = qMax(1, historyMax);

    pDeltaTimer->start();
}

//Destructor
LowPassFilterProcess::~LowPassFilterProcess()
{

}

//Process the input image and return a filtered output
QImage LowPassFilterProcess::process(QImage inputImage)
{
    addToHistory(inputImage);

    QImage previousImage = pPreviousImages.dequeue();
    quint64 deltaTime = pDeltaTimer->restart();

    //TODO: Fully implement this

    float ePow = 1 - qExp(-deltaTime * 2 * PI * pFilterCutoff);
    //Simple LP filter simply mixes current and previous samples (this will halve the effective update speed though)
    BYTE* inputArray = inputImage.bits();
    BYTE* previousArray = previousImage.bits();
    qint64 arrayLength = inputImage.sizeInBytes();

    for (qint32 i = 0; i < arrayLength; ++i)
    {
        BYTE previousByte = previousArray[i];
        BYTE inputByte = inputArray[i];
        BYTE calculatedByte = static_cast<BYTE>(static_cast<float>(inputByte - previousByte) * ePow);
        previousArray[i] += calculatedByte;
    }

    return previousImage;
}

//If newest image is different than previous ones, flush because calculations won't make sense
void LowPassFilterProcess::flushHistoryIfIncompatible(QImage &inputImage)
{
    if (pPreviousImages.constFirst().size() != inputImage.size())
        pPreviousImages.clear();
}

void LowPassFilterProcess::addToHistory(QImage &inputImage)
{
    flushHistoryIfIncompatible(inputImage);

    pPreviousImages.enqueue(inputImage);
    while (pPreviousImages.length() > pHistoryMax)
    {
        pPreviousImages.dequeue();
    }
}


