#include "blockstriptransform.h"
#include "qmath.h"

//Constructor
BlockStripTransform::BlockStripTransform(float stripWidthPercentage) : ImageTransform()
{
    Q_ASSERT(stripWidthPercentage > 0.0f && stripWidthPercentage < 1.0f);
    pStripWidthPercentage = std::clamp(stripWidthPercentage, 0.0f, 1.0f);
}

//Destructor
BlockStripTransform::~BlockStripTransform()
{

}

//Average an image
QImage BlockStripTransform::averageStrip(const QImage &input, ImageSize size, StripPlacement placement)
{
    bool isHorizontalStrip = placement == TopStrip || placement == BottomStrip;
    //"Width" is used in the general sense of the smaller of two lengths of a rectangle
    int stripWidthPx = isHorizontalStrip ? qCeil(static_cast<float>(input.height()) * pStripWidthPercentage) : qCeil(static_cast<float>(input.width()) * pStripWidthPercentage);

    //Grab a strip from the placement side of the screen
    QImage output = copyStrip(input, stripWidthPx, placement);
    //Scale that fast to an intermediate size
    output = isHorizontalStrip ? output.scaled(input.width() / 2, stripWidthPx, Qt::IgnoreAspectRatio, Qt::FastTransformation) : output.scaled(stripWidthPx, input.height() / 2, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    //Scale nicely to final size
    return output.scaled(size.width, size.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
