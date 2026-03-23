#include <gtest/gtest.h>
#include "lowpassfilterprocess.h"
#include <QImage>

namespace {

QImage solidImage(int w, int h, Qt::GlobalColor colour)
{
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(colour);
    return img;
}

} // namespace

// Regression: calling process() on a fresh filter (empty history queue) must
// not crash. Fixed by guarding constFirst() with an isEmpty() check.
TEST(LowPassFilterProcess, NoCrashOnFirstFrame)
{
    LowPassFilterProcess filter;
    QImage input = solidImage(16, 1, Qt::red);
    EXPECT_NO_FATAL_FAILURE(filter.process(input));
}

// Output dimensions must always match the input.
TEST(LowPassFilterProcess, OutputSizeMatchesInput)
{
    LowPassFilterProcess filter;
    QImage input = solidImage(32, 1, Qt::blue);
    QImage output = filter.process(input);
    EXPECT_EQ(output.width(),  input.width());
    EXPECT_EQ(output.height(), input.height());
}

// Changing the image size between calls must not crash (history is flushed).
TEST(LowPassFilterProcess, HandlesResolutionChange)
{
    LowPassFilterProcess filter;
    filter.process(solidImage(8, 1, Qt::red));
    EXPECT_NO_FATAL_FAILURE(filter.process(solidImage(16, 1, Qt::blue)));
}

// Output pixel values must stay within the valid [0, 255] byte range.
TEST(LowPassFilterProcess, OutputValuesInRange)
{
    LowPassFilterProcess filter;
    QImage input = solidImage(16, 1, Qt::white);
    QImage output;
    for (int i = 0; i < 10; ++i)
        output = filter.process(input);

    for (int x = 0; x < output.width(); ++x)
    {
        QRgb pixel = output.pixel(x, 0);
        EXPECT_GE(qRed(pixel),   0);   EXPECT_LE(qRed(pixel),   255);
        EXPECT_GE(qGreen(pixel), 0);   EXPECT_LE(qGreen(pixel), 255);
        EXPECT_GE(qBlue(pixel),  0);   EXPECT_LE(qBlue(pixel),  255);
    }
}
