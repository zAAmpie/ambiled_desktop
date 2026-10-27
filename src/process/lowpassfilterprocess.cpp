#include "lowpassfilterprocess.h"
#include "qmath.h"
#include "types.h"
#include "util.h"

//Constructor
LowPassFilterProcess::LowPassFilterProcess(int filterCutoff) : ImageProcess(), pStateFormat(QImage::Format_Invalid)
{
    Q_ASSERT(filterCutoff > 0 && filterCutoff <= 20000); //Warn at >20kHz, unlikely to make any difference
    pFilterCutoff = qMax(0, filterCutoff);

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

    return {cutoff};
}

QVariantMap LowPassFilterProcess::parameters() const
{
    QVariantMap parameters;
    parameters.insert(QStringLiteral("filterCutoffHz"), pFilterCutoff);
    return parameters;
}

void LowPassFilterProcess::setParameters(const QVariantMap &parameters)
{
    if (parameters.contains(QStringLiteral("filterCutoffHz")))
        pFilterCutoff = qMax(1, parameters.value(QStringLiteral("filterCutoffHz")).toInt());
}

//Process the input image and return a filtered output
QImage LowPassFilterProcess::process(QImage inputImage)
{
    float deltaTimeSeconds = static_cast<float>(pDeltaTimer->restart()) / 1000.0f;

    int rowBytes = inputImage.width() * (inputImage.depth() >> 3);
    qsizetype stateLength = static_cast<qsizetype>(rowBytes) * inputImage.height();

    //No compatible state yet (first frame, or the strips changed size/format) - start from this frame
    if (pState.size() != stateLength || pStateSize != inputImage.size() || pStateFormat != inputImage.format())
    {
        pState.resize(stateLength);
        float *state = pState.data();
        for (int y = 0; y < inputImage.height(); ++y)
        {
            const BYTE *line = inputImage.constScanLine(y);
            for (int i = 0; i < rowBytes; ++i)
                *state++ = line[i];
        }
        pStateSize = inputImage.size();
        pStateFormat = inputImage.format();
        return inputImage;
    }

    //Fraction of the way the output moves towards the input this frame - for a first-order low pass
    //with cutoff fc this is 1 - e^(-dt * 2 * pi * fc), so it adapts to however long the frame took
    float alpha = 1.0f - qExp(-deltaTimeSeconds * 2.0f * static_cast<float>(PI) * pFilterCutoff);

    QImage outputImage(inputImage.size(), inputImage.format());
    float *state = pState.data();
    for (int y = 0; y < inputImage.height(); ++y)
    {
        const BYTE *inputLine = inputImage.constScanLine(y);
        BYTE *outputLine = outputImage.scanLine(y);
        for (int i = 0; i < rowBytes; ++i)
        {
            *state += (static_cast<float>(inputLine[i]) - *state) * alpha;
            outputLine[i] = static_cast<BYTE>(qRound(*state));
            ++state;
        }
    }

    return outputImage;
}


