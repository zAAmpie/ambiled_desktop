#include "blackbarprocess.h"
#include <QImage>
#include "qmath.h"
#include "types.h"

//Constructor
BlackBarProcess::BlackBarProcess(float scanThresholdPercentage, float blackValueThreshold) : ImageProcess()
{
    Q_ASSERT(scanThresholdPercentage > 0.0f && scanThresholdPercentage <= 1.0f);
    Q_ASSERT(blackValueThreshold >= 0.0f && blackValueThreshold <= 255.0f);
    pScanThreshold = std::clamp(scanThresholdPercentage, 0.0f, 1.0f);
    pValueThreshold = std::clamp(blackValueThreshold, 0.0f, 255.0f); //Assume 8-bit colour!
}

//Destructor
BlackBarProcess::~BlackBarProcess()
{
}

//Process the image and remove black bars
QImage BlackBarProcess::process(const QImage& inputImage)
{
    Q_ASSERT(!inputImage.isNull());

    //Try and remove any black bars on the top and bottom
    int topLineSkip = getFirstNonBlackLine(inputImage);
    int bottomLineSkip = inputImage.height() - getLastNonBlackLine(inputImage) - 1;
    int reducedHeight = inputImage.height() - bottomLineSkip - topLineSkip;

    //Early out if it's the same height
    if (reducedHeight == inputImage.height())
        return inputImage;

    //Set some constants
    int scanLineBytes = inputImage.width() * (inputImage.depth() >> 3); //Number of bytes in a scanline

    return QImage(inputImage.constBits() + topLineSkip * scanLineBytes, inputImage.width(), reducedHeight, QImage::Format_RGB32);
}

// Scan top-down and return the index of the first non-black row.
// Each row is sampled at COLUMN_SAMPLE_STEP intervals; pixels are read as uint32_t
// (Format_RGB32 = 0xffRRGGBB) so alpha is naturally excluded and the inner per-byte
// loop is eliminated. The inner loop exits early once sum exceeds the threshold.
int BlackBarProcess::getFirstNonBlackLine(const QImage& image)
{
    const int width = image.width();
    int idxLine = 0;
    int stopLine = qCeil(static_cast<double>(image.height()) * pScanThreshold);

    // Scale threshold to the number of sampled pixels; multiply by 3 for RGB channels only
    int sampledPixels = (width + COLUMN_SAMPLE_STEP - 1) / COLUMN_SAMPLE_STEP;
    int blackThreshold = static_cast<int>(pValueThreshold * sampledPixels * 3);
    int sum;

    do
    {
        sum = 0;
        const auto* pixel = reinterpret_cast<const uint32_t*>(image.constScanLine(idxLine++));
        const auto* end   = pixel + width;

        for (; pixel < end; pixel += COLUMN_SAMPLE_STEP)
        {
            uint32_t px = *pixel;
            sum += (px & 0xFF) + ((px >> 8) & 0xFF) + ((px >> 16) & 0xFF); // B + G + R
            if (sum >= blackThreshold)
                break; // Row is non-black — no need to scan further
        }
    } while (idxLine < stopLine && sum < blackThreshold);

    return idxLine - 1;
}

// Scan bottom-up and return the index of the last non-black row.
int BlackBarProcess::getLastNonBlackLine(const QImage& image)
{
    const int width = image.width();
    int idxLine = image.height() - 1;
    int stopLine = qFloor(static_cast<double>(image.height()) * (1.0 - pScanThreshold));

    int sampledPixels = (width + COLUMN_SAMPLE_STEP - 1) / COLUMN_SAMPLE_STEP;
    int blackThreshold = static_cast<int>(pValueThreshold * sampledPixels * 3);
    int sum;

    do
    {
        sum = 0;
        const auto* pixel = reinterpret_cast<const uint32_t*>(image.constScanLine(idxLine--));
        const auto* end   = pixel + width;

        for (; pixel < end; pixel += COLUMN_SAMPLE_STEP)
        {
            uint32_t px = *pixel;
            sum += (px & 0xFF) + ((px >> 8) & 0xFF) + ((px >> 16) & 0xFF); // B + G + R
            if (sum >= blackThreshold)
                break; // Row is non-black — no need to scan further
        }
    } while (idxLine > stopLine && sum < blackThreshold);

    return idxLine + 1;
}
