#include <gtest/gtest.h>
#include "leds.h"

// ---------------------------------------------------------------------------
// transformLuxToBrightness (static)
// ---------------------------------------------------------------------------

TEST(LEDS, LuxBrightnessClampedToMin)
{
    EXPECT_EQ(LEDS::transformLuxToBrightness(0),    5);
    EXPECT_EQ(LEDS::transformLuxToBrightness(-100), 5);
}

TEST(LEDS, LuxBrightnessClampedToMax)
{
    EXPECT_EQ(LEDS::transformLuxToBrightness(255),  255);
    EXPECT_EQ(LEDS::transformLuxToBrightness(1000), 255);
}

TEST(LEDS, LuxBrightnessPassthroughInRange)
{
    EXPECT_EQ(LEDS::transformLuxToBrightness(100), 100);
    EXPECT_EQ(LEDS::transformLuxToBrightness(128), 128);
}

// ---------------------------------------------------------------------------
// Brightness
// ---------------------------------------------------------------------------

TEST(LEDS, DefaultBrightnessIsOne)
{
    LEDS leds;
    EXPECT_DOUBLE_EQ(leds.getBrightness(), 1.0);
}

TEST(LEDS, BrightnessRoundtrip)
{
    LEDS leds;
    leds.setBrightness(0.75);
    EXPECT_DOUBLE_EQ(leds.getBrightness(), 0.75);
}

TEST(LEDS, BrightnessZero)
{
    LEDS leds;
    leds.setBrightness(0.0);
    EXPECT_DOUBLE_EQ(leds.getBrightness(), 0.0);
}

// ---------------------------------------------------------------------------
// Colour temperature
// ---------------------------------------------------------------------------

TEST(LEDS, ColourTemperatureRGBEncoding)
{
    LEDS leds;
    leds.setColourTemperature(255, 128, 64);
    int temp = leds.getColourTemperature();
    EXPECT_EQ((temp >> 16) & 0xFF, 255);
    EXPECT_EQ((temp >> 8)  & 0xFF, 128);
    EXPECT_EQ( temp        & 0xFF,  64);
}

TEST(LEDS, ColourTemperaturePresetUncorrected)
{
    LEDS leds;
    leds.setColourTemperature(LEDS::UncorrectedTemperature);
    int temp = leds.getColourTemperature();
    EXPECT_EQ((temp >> 16) & 0xFF, 255);
    EXPECT_EQ((temp >> 8)  & 0xFF, 255);
    EXPECT_EQ( temp        & 0xFF, 255);
}

// ---------------------------------------------------------------------------
// Strip sizes derived from LED positions
// ---------------------------------------------------------------------------

TEST(LEDS, GetStripSizesMatchesPositions)
{
    LEDS leds;
    LEDPositions pos;
    pos.t = 30; pos.l = 20; pos.r = 20;
    pos.bl = 10; pos.br = 10; pos.bg = 5;
    leds.setPositions(pos);

    StripSizes sizes = leds.getStripSizes();
    EXPECT_EQ(sizes.top,    30);
    EXPECT_EQ(sizes.left,   20);
    EXPECT_EQ(sizes.right,  20);
    EXPECT_EQ(sizes.bottom, 25); // bl + br + bg
}

TEST(LEDS, GetStripSizesDefaultZero)
{
    LEDS leds; // positions default to zero
    StripSizes sizes = leds.getStripSizes();
    EXPECT_EQ(sizes.top,    0);
    EXPECT_EQ(sizes.bottom, 0);
    EXPECT_EQ(sizes.left,   0);
    EXPECT_EQ(sizes.right,  0);
}
