#include "blackbarprocess.h"
#include <QImage>
#include "qmath.h"
#include "types.h"

//Constructor
BlackBarProcess::BlackBarProcess(float scanThresholdPercentage, float blackValueThreshold) : pScanThreshold(scanThresholdPercentage), pValueThreshold(blackValueThreshold)
{

}

//Destructor
BlackBarProcess::~BlackBarProcess()
{

}

//Process the image and remove black bars
QImage BlackBarProcess::process(QImage inputImage)
{
    //Try and remove any black bars on the top and bottom
    int topLineSkip = getFirstNonBlackLine(inputImage);
    int bottomLineSkip = getLastNonBlackLine(inputImage);
    int reducedHeight = inputImage.height() - bottomLineSkip - topLineSkip;

    //Set some constants
    int scanLineBytes = inputImage.width() * (inputImage.depth() >> 3); //Number of bytes in a scanline

    return QImage(inputImage.constBits() + topLineSkip * scanLineBytes, inputImage.width(), reducedHeight, QImage::Format_RGB32);;
}

//Test to see if line is black (within a theshold)
int BlackBarProcess::getFirstNonBlackLine(const QImage image)
{
    int idxLine = 0;
    int stopLine = qCeil(static_cast<double>(image.height()) * pScanThreshold);
    int depth = image.depth() >> 3; //Divide by 8 to get bytes
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

    return idxLine;
}

int BlackBarProcess::getLastNonBlackLine(const QImage image)
{
    int idxLine = image.height() - 1;
    int stopLine = qFloor(static_cast<double>(image.height()) * (1 - pScanThreshold));
    int depth = image.depth() >> 3; //Divide by 8 to get bytes
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

    return idxLine;
}

