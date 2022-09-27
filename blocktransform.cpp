#include "blocktransform.h"
#include "qmath.h"

//Constructor
BlockTransform::BlockTransform(float stripWidthPercentage) : ImageTransform()
{
    Q_ASSERT(stripWidthPercentage > 0.0f && stripWidthPercentage < 1.0f);
    pStripWidthPercentage = std::clamp(stripWidthPercentage, 0.0f, 1.0f);
}

//Destructor
BlockTransform::~BlockTransform()
{

}

//Function that transform an input image into strips
ImageStrips BlockTransform::transform(QImage inputImage, const StripSizes sizes)
{
    ImageStrips outStrips;

    //Do each of the strips
    outStrips.topLine = averageStrip(inputImage, ImageSize(1, sizes.top), StripPlacement::TopStrip);
    outStrips.bottomLine = averageStrip(inputImage, ImageSize(1, sizes.bottom), StripPlacement::BottomStrip);
    outStrips.leftLine = averageStrip(inputImage, ImageSize(1, sizes.left), StripPlacement::LeftStrip);
    outStrips.rightLine = averageStrip(inputImage, ImageSize(1, sizes.right), StripPlacement::RightStrip);

    return outStrips;
}

//Average an image
QImage BlockTransform::averageStrip(const QImage &input, ImageSize size, StripPlacement placement)
{
    bool isHorizontalStrip = placement == TopStrip || placement == BottomStrip;
    //"Width" is used in the general sense of the smaller of two lengths of a rectangle
    int stripWidthPx = isHorizontalStrip ? qCeil(static_cast<float>(input.width()) * pStripWidthPercentage) : qCeil(static_cast<float>(input.height()) * pStripWidthPercentage);

    //Grab a strip from the placement side of the screen
    QImage output = copyStrip(input, stripWidthPx, placement);
    //Scale nicely to final size
    return output.scaled(size.width, size.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
