#include "simpletransform.h"

//Constructor
SimpleTransform::SimpleTransform() : ImageTransform()
{

}

//Destructor
SimpleTransform::~SimpleTransform()
{

}

//Function that transform an input image into strips
ImageStrips SimpleTransform::transform(QImage inputImage, const StripSizes sizes)
{
    Q_ASSERT(!inputImage.isNull());
    Q_ASSERT(sizes.isSet());

    ImageStrips outStrips;

    //Do each of the strips
    outStrips.topLine = averageStrip(inputImage, ImageSize(1, sizes.top), StripPlacement::TopStrip);
    outStrips.bottomLine = averageStrip(inputImage, ImageSize(1, sizes.bottom), StripPlacement::BottomStrip);
    outStrips.leftLine = averageStrip(inputImage, ImageSize(1, sizes.left), StripPlacement::LeftStrip);
    outStrips.rightLine = averageStrip(inputImage, ImageSize(1, sizes.right), StripPlacement::RightStrip);

    return outStrips;
}

//Average an image
QImage SimpleTransform::averageStrip(const QImage &input, ImageSize size, StripPlacement placement)
{
    //Grab a 1px strip from the placement side of the screen
    QImage output = copyStrip(input, 1, placement);
    //Scale fast to final size
    return output.scaled(size.width, size.height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}
