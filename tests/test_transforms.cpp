#include <gtest/gtest.h>
#include "simpletransform.h"
#include "striptransform.h"
#include "blocktransform.h"
#include "blockstriptransform.h"
#include <QImage>

namespace {

QImage solidImage(int w, int h, Qt::GlobalColor colour)
{
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(colour);
    return img;
}

// Verify all four strip dimensions for a given transform and strip configuration.
void checkStripDimensions(ImageTransform &transform, QImage input, StripSizes sizes)
{
    ImageStrips strips = transform.transform(input, sizes);
    EXPECT_EQ(strips.topLine.width(),     sizes.top)    << "topLine width";
    EXPECT_EQ(strips.topLine.height(),    1)             << "topLine height";
    EXPECT_EQ(strips.bottomLine.width(),  sizes.bottom)  << "bottomLine width";
    EXPECT_EQ(strips.bottomLine.height(), 1)             << "bottomLine height";
    EXPECT_EQ(strips.leftLine.width(),    1)             << "leftLine width";
    EXPECT_EQ(strips.leftLine.height(),   sizes.left)    << "leftLine height";
    EXPECT_EQ(strips.rightLine.width(),   1)             << "rightLine width";
    EXPECT_EQ(strips.rightLine.height(),  sizes.right)   << "rightLine height";
}

// For a uniformly coloured input, all strips should approximate that colour.
void checkUniformColour(ImageTransform &transform, Qt::GlobalColor colour)
{
    QImage input = solidImage(200, 120, colour);
    StripSizes sizes(20, 20, 15, 15);
    ImageStrips strips = transform.transform(input, sizes);

    QColor expected(colour);
    auto close = [](int a, int b) { return std::abs(a - b) < 30; };

    for (const QImage *strip : {&strips.topLine, &strips.bottomLine,
                                 &strips.leftLine, &strips.rightLine})
    {
        QRgb p = strip->pixel(0, 0);
        EXPECT_TRUE(close(qRed(p),   expected.red()))   << "red channel off";
        EXPECT_TRUE(close(qGreen(p), expected.green())) << "green channel off";
        EXPECT_TRUE(close(qBlue(p),  expected.blue()))  << "blue channel off";
    }
}

} // namespace

// ---------------------------------------------------------------------------
// SimpleTransform
// ---------------------------------------------------------------------------

TEST(SimpleTransform, OutputDimensions)
{
    SimpleTransform t;
    checkStripDimensions(t, solidImage(200, 120, Qt::red), StripSizes(30, 30, 20, 20));
}

TEST(SimpleTransform, UniformColourPreserved)
{
    SimpleTransform t;
    checkUniformColour(t, Qt::red);
}

TEST(SimpleTransform, TopEdgeSampledFromFirstRow)
{
    SimpleTransform t;
    // Top row is red, everything else is blue
    QImage input = solidImage(200, 120, Qt::blue);
    for (int x = 0; x < 200; ++x)
        input.setPixel(x, 0, qRgb(255, 0, 0));

    ImageStrips strips = t.transform(input, StripSizes(10, 10, 10, 10));
    QRgb top = strips.topLine.pixel(0, 0);
    EXPECT_GT(qRed(top),   200);
    EXPECT_LT(qBlue(top),  50);
}

TEST(SimpleTransform, BottomEdgeSampledFromLastRow)
{
    SimpleTransform t;
    // Bottom row is green, everything else is blue
    QImage input = solidImage(200, 120, Qt::blue);
    for (int x = 0; x < 200; ++x)
        input.setPixel(x, 119, qRgb(0, 255, 0));

    ImageStrips strips = t.transform(input, StripSizes(10, 10, 10, 10));
    QRgb bottom = strips.bottomLine.pixel(0, 0);
    EXPECT_GT(qGreen(bottom), 200);
    EXPECT_LT(qBlue(bottom),  50);
}

// ---------------------------------------------------------------------------
// StripTransform
// ---------------------------------------------------------------------------

TEST(StripTransform, OutputDimensions)
{
    StripTransform t;
    checkStripDimensions(t, solidImage(200, 120, Qt::green), StripSizes(30, 30, 20, 20));
}

TEST(StripTransform, UniformColourPreserved)
{
    StripTransform t;
    checkUniformColour(t, Qt::blue);
}

// ---------------------------------------------------------------------------
// BlockTransform
// ---------------------------------------------------------------------------

TEST(BlockTransform, OutputDimensions)
{
    BlockTransform t;
    checkStripDimensions(t, solidImage(200, 120, Qt::blue), StripSizes(30, 30, 20, 20));
}

TEST(BlockTransform, UniformColourPreserved)
{
    BlockTransform t;
    checkUniformColour(t, Qt::green);
}

// ---------------------------------------------------------------------------
// BlockStripTransform
// ---------------------------------------------------------------------------

TEST(BlockStripTransform, OutputDimensions)
{
    BlockStripTransform t;
    checkStripDimensions(t, solidImage(200, 120, Qt::red), StripSizes(30, 30, 20, 20));
}

TEST(BlockStripTransform, UniformColourPreserved)
{
    BlockStripTransform t;
    checkUniformColour(t, Qt::red);
}
