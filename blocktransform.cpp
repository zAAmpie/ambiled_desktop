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

//Average an image
QImage BlockTransform::averageStrip(const QImage &input, ImageSize size, StripPlacement placement)
{
    bool isHorizontalStrip = placement == TopStrip || placement == BottomStrip;
    //"Width" is used in the general sense of the smaller of two lengths of a rectangle
    int stripWidthPx = isHorizontalStrip ? qCeil(static_cast<float>(input.height()) * pStripWidthPercentage) : qCeil(static_cast<float>(input.width()) * pStripWidthPercentage);

    //Grab a strip from the placement side of the screen
    QImage output = copyStrip(input, stripWidthPx, placement);
    //Scale nicely to final size
    return output.scaled(size.width, size.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
