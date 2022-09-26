#pragma once

#include <QImage>
#include "util.h"

/* Header file containing everything to do with the LEDs, i.e.
 * - Positions
 * - Manipulation functions
 * - Converting QImage to an LED representation */

class ImageStrips
{
    /* Larger representation of input lines serving as input to the LED transformation
     * Essentially a 1xN or Nx1 image for each edge of the screen */
public:
    QImage topLine;
    QImage bottomLine;
    QImage leftLine;
    QImage rightLine;

    bool isNull() const
    {
        return topLine.isNull() || bottomLine.isNull() || leftLine.isNull() || rightLine.isNull();
    }
};

class LEDPositions
{
    /* Class that contains the positions and division of the LED strip around the TV/monitor */
public:
    //Positions as viewed from FRONT of monitor
    // -------- t ---------
    // |                  |
    // l                  r
    // |                  |
    // -- bl -- bg -- br --

    int br; //Bottom Right
    int r; //Right
    int t; //Top
    int l; //Left
    int bl; //Bottom Left
    int bg; //Bottom Gap

    int getBottomCount() const {return bl + br + bg;}

    int getLength() const {return br + r + t + l + bl + bg;} //Everything except gap since they are at the end of the string

    bool isNull() const
    {
        return br && r && l && t && bl;
    }

    //Just initialize to zero on creation
    LEDPositions() : br(0), r(0), t(0), l(0), bl(0), bg(0) {}
};

class LEDS
{
    /* General class controlling the LEDs in terms of representation and transformation */
public:
    LEDS();
    ~LEDS();

    typedef enum {
        //Black body radiation colours
        Candle = 0xFF9329 /* 1900 K, 255, 147, 41 */,
        Tungsten40W = 0xFFC58F /* 2600 K, 255, 197, 143 */,
        Tungsten100W = 0xFFD6AA /* 2850 K, 255, 214, 170 */,
        Halogen = 0xFFF1E0 /* 3200 K, 255, 241, 224 */,
        CarbonArc = 0xFFFAF4 /* 5200 K, 255, 250, 244 */,
        HighNoonSun = 0xFFFFFB /* 5400 K, 255, 255, 251 */,
        DirectSunlight = 0xFFFFFF /* 6000 K, 255, 255, 255 */,
        OvercastSky = 0xC9E2FF /* 7000 K, 201, 226, 255 */,
        ClearBlueSky = 0x409CFF /* 20000 K, 64, 156, 255 */,

        //Gaseous light source colours
        WarmFluorescent = 0xFFF4E5 /* 0 K, 255, 244, 229 */,
        StandardFluorescent = 0xF4FFFA /* 0 K, 244, 255, 250 */,
        CoolWhiteFluorescent = 0xD4EBFF /* 0 K, 212, 235, 255 */,
        FullSpectrumFluorescent = 0xFFF4F2 /* 0 K, 255, 244, 242 */,
        GrowLightFluorescent = 0xFFEFF7 /* 0 K, 255, 239, 247 */,
        BlackLightFluorescent = 0xA700FF /* 0 K, 167, 0, 255 */,
        MercuryVapor = 0xD8F7FF /* 0 K, 216, 247, 255 */,
        SodiumVapor = 0xFFD1B2 /* 0 K, 255, 209, 178 */,
        MetalHalide = 0xF2FCFF /* 0 K, 242, 252, 255 */,
        HighPressureSodium = 0xFFB74C /* 0 K, 255, 183, 76 */,

        UncorrectedTemperature = 0xFFFFFF
    } ColourTemperature;

    typedef enum {
        Running = qRgb(0, 16, 0),
        Error = qRgb(255, 0, 0)
    } LEDStatus;

    //Transform image represenation to LED representation
    void updateLED(const ImageStrips& lines);

    //Basic function to transform lux to a brightness value
    static int transformLuxToBrightness(int lux);

    //Set new LED positions
    void setPositions(LEDPositions newPositions);

    //Set LED temperature adjustment
    void setColourTemperature(int temperature);
    void setColourTemperature(ColourTemperature temperature);
    void setColourTemperature(int red, int green, int blue);

    //Set the LED brightness
    void setBrightness(double brightness);

    //Set the LED status
    void setStatus(LEDStatus status);

    //Get the LED positions if needed - shouldn't really be required
    LEDPositions getPositions() const {return pPositions;}

    //Get current colour temperature setting
    int getColourTemperature() const {return pColourTemperature;}

    //Get the LED representation
    QImage getStrip();

    //Get the current LED brightness
    double getBrightness() const {return pBrightness;}

    //Get status
    LEDStatus getStatus() const {return pStatus;}

    //Get a test LED representation
    QPair<QImage, ImageStrips> generateTestPattern();

private:
    const int LEDStripExcess = 10;
    //Transform the main representation and return a brightness adjusted version
    QImage transformBrightness(QImage inputImage, double brightness, ColourTemperature temperature);

    //Transform image represenation to LED representation
    QImage transformToLED(const ImageStrips& lines);

    //Private constants
    QImage pLEDImage; //The main representation of the output LED strip in memory
    LEDPositions pPositions; //The representation of how the strip is segmented
    bool pInitialised; //Has the LED strip been initialised?
    int pColourTemperature; //The current temperature
    double pBrightness; //Current brightness (0-1)
    LEDStatus pStatus; //Current status
    int pLEDStripLength; //The length of each LED strip
};

