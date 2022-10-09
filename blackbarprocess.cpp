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
QImage BlackBarProcess::process(QImage inputImage)
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

    return QImage(inputImage.constBits() + topLineSkip * scanLineBytes, inputImage.width(), reducedHeight, QImage::Format_RGB32);;
}

//Test to see if line is black (within a theshold)
int BlackBarProcess::getFirstNonBlackLine(const QImage image)
{
    int idxLine = 0;
    int stopLine = qCeil(static_cast<double>(image.height()) * pScanThreshold);
    //TODO: Support for 10-bit colour etc.
    int depth = image.depth() >> 3; //Divide by 8 to get bytes - assume 8-bit colour
    int scanLinePitch = image.width() * depth;
    int blackThreshold = pValueThreshold * scanLinePitch;
    int sum;

    //Determine which layer is the alpha
    int skipAlphaIndex = 0;
    if (image.pixelFormat().alphaPosition() == QPixelFormat::AtEnd)
        skipAlphaIndex = depth - 1;

    do
    {
        sum = 0;
        //Get pointer to first scanline
        const BYTE *line = image.constScanLine(idxLine++);

        //Advance pointer in increments of depth until end of data
        for (int ptrOffset = 0; ptrOffset < scanLinePitch; ptrOffset += depth)
        {
            //Sum value of RGB for each pixel
            for (int colour = 0; colour < depth; ++colour)
            {
                if (colour == skipAlphaIndex)
                    continue;
                sum += *line++;
            }
        }
    } while (idxLine < stopLine && sum < blackThreshold);

    return idxLine - 1;
}

int BlackBarProcess::getLastNonBlackLine(const QImage image)
{
    int idxLine = image.height() - 1;
    int stopLine = qFloor(static_cast<double>(image.height()) * (1 - pScanThreshold));
    //TODO: Support for 10-bit colour
    int depth = image.depth() >> 3; //Divide by 8 to get bytes - assume 8-bit colour
    int scanLinePitch = image.width() * depth;
    int blackThreshold = pValueThreshold * scanLinePitch;
    int sum;

    //Determine which layer is the alpha
    int skipAlphaIndex = 0;
    if (image.pixelFormat().alphaPosition() == QPixelFormat::AtEnd)
        skipAlphaIndex = depth - 1;

    do
    {
        sum = 0;
        //Get pointer to first scanline
        const BYTE *line = image.constScanLine(idxLine--);

        //Advance pointer in increments of depth until end of data
        for (int ptrOffset = 0; ptrOffset < scanLinePitch; ptrOffset += depth)
        {
            //Sum value of RGB for each pixel
            for (int colour = 0; colour < depth; ++colour)
            {
                if (colour == skipAlphaIndex)
                    continue;
                sum += *line++;
            }
        }
    } while (idxLine > stopLine && sum < blackThreshold);

    return idxLine + 1;
}

