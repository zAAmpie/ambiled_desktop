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

    //Start delta timer for the first run
    pDeltaTimer = new QElapsedTimer();
    pDeltaTimer->start();
}

//Destructor
LowPassFilterProcess::~LowPassFilterProcess()
{
    delete pDeltaTimer;
}

//Generic configuration contract (see ImageProcess)
QList<ImageProcess::ParameterSpec> LowPassFilterProcess::parameterSpecs()
{
    ImageProcess::ParameterSpec cutoff;
    cutoff.key = QStringLiteral("filterCutoffHz");
    cutoff.label = QStringLiteral("Cutoff frequency");
    cutoff.minimum = 1;
    cutoff.maximum = 60;
    cutoff.suffix = QStringLiteral(" Hz");

    ImageProcess::ParameterSpec history;
    history.key = QStringLiteral("historyMax");
    history.label = QStringLiteral("History length");
    history.minimum = 1;
    history.maximum = 10;

    return {cutoff, history};
}

QVariantMap LowPassFilterProcess::parameters() const
{
    QVariantMap parameters;
    parameters.insert(QStringLiteral("filterCutoffHz"), pFilterCutoff);
    parameters.insert(QStringLiteral("historyMax"), pHistoryMax);
    return parameters;
}

void LowPassFilterProcess::setParameters(const QVariantMap &parameters)
{
    if (parameters.contains(QStringLiteral("filterCutoffHz")))
        pFilterCutoff = qMax(1, parameters.value(QStringLiteral("filterCutoffHz")).toInt());
    if (parameters.contains(QStringLiteral("historyMax")))
        pHistoryMax = qMax(1, parameters.value(QStringLiteral("historyMax")).toInt());
}

//Process the input image and return a filtered output
QImage LowPassFilterProcess::process(QImage inputImage)
{
    flushHistoryIfIncompatible(inputImage);

    //Diff against the oldest frame still in history (i.e. historyMax frames back), or the
    //current frame itself on the very first call when there's no history yet
    QImage previousImage = pPreviousImages.isEmpty() ? inputImage : pPreviousImages.constFirst();

    addToHistory(inputImage);

    quint64 deltaTimeMs = pDeltaTimer->restart();
    float deltaTimeSeconds = static_cast<float>(deltaTimeMs) / 1000.0f;

    float ePow = 1 - qExp(-deltaTimeSeconds * 2 * PI * pFilterCutoff);
    //Simple LP filter simply mixes current and previous samples (this will halve the effective update speed though)
    BYTE* inputArray = inputImage.bits();  //TODO: bits() does deep-copy
    BYTE* previousArray = previousImage.bits();  //TODO: bits() does deep-copy
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
    if (!pPreviousImages.isEmpty() && pPreviousImages.constFirst().size() != inputImage.size())
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


