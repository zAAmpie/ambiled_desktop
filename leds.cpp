#include "leds.h"

LEDS::LEDS()
{
    pBrightness = 1.0;
    pColourTemperature = static_cast<int>(ColourTemperature::UncorrectedTemperature);
    pStatus = LEDStatus::Running;
    pLEDStripLength = 120;
}

LEDS::~LEDS()
{

}

//Update LED data
void LEDS::updateLED(const ImageStrips& lines)
{
    Q_ASSERT(!lines.isNull());
    pLEDImage = transformToLED(lines);
}

//Transform image represenation to LED representation
QImage LEDS::transformToLED(const ImageStrips& lines)
{
    /*
    Screen is shown from front

    LEDs are done clockwise from front

    //Positions as viewed from FRONT of monitor
    // -------- t ---------
    // |                  |
    // l                  r
    // |                  |
    // -- bl -- bg -- br --

    Packing order is:

    (Old) (clockwise)
    bottom left (rtl) --> left (btt) --> top (ltr) --> right (ttb) --> bottom right (rtl)

    (New dual channel mode) (first left half, then right half)
    bottom left (rtl) --> left (btt) --> left 50% of top (ltr) --> bottom right (ltr) --> right (btt) --> right 50% of top (rtl)

    For QImage:

    rtl = reverse
    btt = reverse
    ltr = normal
    ttb = normal

    */
    QImage data(pPositions.getLength() + LEDStripExcess, 1, QImage::Format_RGB32);
    int dataIndex = 0;

    //Start with first status led
    data.setPixel(QPoint(dataIndex++, 0), pStatus);

    for (int i = pPositions.bl - 1; i >= 0; --i)
        data.setPixel(QPoint(dataIndex++, 0), lines.bottomLine.pixel(QPoint(i, 0)));

    //Append left
    for (int i = pPositions.l - 1; i >= 0; --i)
        data.setPixel(QPoint(dataIndex++, 0), lines.leftLine.pixel(QPoint(0, i)));

    //Append left 50% of top
    int topBreak = pPositions.t / 2;
    for (int i = 0; i < topBreak; ++i)
        data.setPixel(QPoint(dataIndex++, 0), lines.topLine.pixel(QPoint(i, 0)));

    //Append extra for additional end of strip
    for (int i = 0; i < pLEDStripLength - topBreak - pPositions.l - pPositions.bl - 1; ++i)
        data.setPixel(QPoint(dataIndex++, 0), qRgb(0, 0, 0));

    //Start with first status led
    data.setPixel(QPoint(dataIndex++, 0), pStatus);

    //Append bottom right
    for (int i = pPositions.getBottomCount() - pPositions.br; i < pPositions.getBottomCount(); ++i)
        data.setPixel(QPoint(dataIndex++, 0), lines.bottomLine.pixel(QPoint(i, 0)));

    //Append right
    for (int i = pPositions.r - 1; i >= 0; --i)
        data.setPixel(QPoint(dataIndex++, 0), lines.rightLine.pixel(QPoint(0, i)));

    //Append right 50% of top
    for (int i = pPositions.t - 1; i >= topBreak; --i)
        data.setPixel(QPoint(dataIndex++, 0), lines.topLine.pixel(QPoint(i, 0)));

    //Append extra for additional end of strip
    for (int i = 0; i < LEDStripExcess; ++i)
        data.setPixel(QPoint(dataIndex++, 0), qRgb(0,0,0));

    return data;
}

//Basic function to transform lux to a brightness value
int LEDS::transformLuxToBrightness(int lux)
{
    //For now, just clamp these to some minimum/maximum value
    return std::clamp(lux, 5, 255);
}

//Set new LED positions
void LEDS::setPositions(LEDPositions newPositions)
{
    pPositions = newPositions;
}

//Get the LED strip sizes
StripSizes LEDS::getStripSizes() const
{
    StripSizes sizes;
    sizes.top = pPositions.t;
    sizes.bottom = pPositions.getBottomCount();
    sizes.left = pPositions.l;
    sizes.right = pPositions.r;
    return sizes;
}

//Set LED temperature adjustment
void LEDS::setColourTemperature(int temperature)
{
    pColourTemperature = temperature;
}

void LEDS::setColourTemperature(ColourTemperature temperature)
{
    setColourTemperature(static_cast<int>(temperature));
}

void LEDS::setColourTemperature(int red, int green, int blue)
{
    //Temperature int is 0xRRGGBB
    setColourTemperature(((red & 0xFF) << 16) | ((green & 0xFF) << 8) | (blue & 0xFF));
}

//Set the LED brightness
void LEDS::setBrightness(double brightness)
{
    Q_ASSERT(brightness >= 0.0 && brightness <= 1.0);

    pBrightness = brightness;
}

//Set the LED status
void LEDS::setStatus(LEDStatus status)
{
    pStatus = status;
}

//Get the LED representation
QImage LEDS::getStrip()
{
    //Do all transformations and return the strip
    return transformBrightness(pLEDImage, pBrightness, pColourTemperature);
}

//Get a test LED representation
QPair<QImage, ImageStrips> LEDS::generateTestPattern()
{
    //Generate test pattern input images
    ImageStrips stripImages;

    //Create the images
    stripImages.bottomLine = QImage(pPositions.getBottomCount(), 1, QImage::Format_RGB32);
    stripImages.topLine = QImage(pPositions.t, 1, QImage::Format_RGB32);
    stripImages.leftLine = QImage(1, pPositions.l, QImage::Format_RGB32);
    stripImages.rightLine = QImage(1, pPositions.r, QImage::Format_RGB32);

    //Fill colours
    stripImages.bottomLine.fill(Qt::red);
    stripImages.topLine.fill(Qt::blue);
    stripImages.leftLine.fill(Qt::cyan);
    stripImages.rightLine.fill(Qt::yellow);

    //Transform input images to LEDs and return both
    return QPair<QImage, ImageStrips>(transformToLED(stripImages), stripImages);
}

//Transform the main representation and return a brightness adjusted version
QImage LEDS::transformBrightness(QImage inputImage, double brightness, int temperature)
{
    Q_ASSERT(inputImage.height() == 1);

    //Calculation correction factors for each colour
    double rCorrection = brightness * static_cast<double>(((temperature >> 16) & 0xFF)) / 255.0;
    double gCorrection = brightness * static_cast<double>(((temperature >> 8) & 0xFF)) / 255.0;
    double bCorrection = brightness * static_cast<double>(((temperature) & 0xFF)) / 255.0;

    //This will create a copy
    QImage correctedImage = inputImage;
    for (int i = 0; i < correctedImage.width(); ++i)
    {
        QColor pixel = correctedImage.pixelColor(i, 0);

        pixel.setRedF(pixel.redF() * rCorrection);
        pixel.setGreenF(pixel.greenF() * gCorrection);
        pixel.setBlueF(pixel.blueF() * bCorrection);

        correctedImage.setPixelColor(i, 0, pixel);
    }

    return correctedImage;
}
