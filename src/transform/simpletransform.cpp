#include "simpletransform.h"

//Constructor
SimpleTransform::SimpleTransform() : ImageTransform()
{

}

//Destructor
SimpleTransform::~SimpleTransform()
{

}

//Average an image
QImage SimpleTransform::averageStrip(const QImage &input, ImageSize size, StripPlacement placement)
{
    //Grab a 1px strip from the placement side of the screen
    QImage output = copyStrip(input, 1, placement);
    //Scale fast to final size
    return output.scaled(size.width, size.height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}
