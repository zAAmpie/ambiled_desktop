#include <gtest/gtest.h>
#include "blackbarprocess.h"
#include <QImage>

namespace {

// Build an image with solid black rows on top/bottom and a coloured content region.
// Use width=200, height>=40 to stay well within the default 25% scan threshold.
QImage imageWithBars(int topBars, int bottomBars, Qt::GlobalColor content)
{
    const int W = 200, H = 100;
    QImage img(W, H, QImage::Format_RGB32);
    img.fill(Qt::black);
    for (int y = topBars; y < H - bottomBars; ++y)
        for (int x = 0; x < W; ++x)
            img.setPixel(x, y, QColor(content).rgb());
    return img;
}

} // namespace

TEST(BlackBarProcess, NoBlackBarsUnchanged)
{
    BlackBarProcess filter;
    QImage input(200, 100, QImage::Format_RGB32);
    input.fill(Qt::red);
    QImage output = filter.process(input);
    EXPECT_EQ(output.width(),  input.width());
    EXPECT_EQ(output.height(), input.height());
}

TEST(BlackBarProcess, CropsTopBars)
{
    BlackBarProcess filter;
    QImage input = imageWithBars(10, 0, Qt::red);
    QImage output = filter.process(input);
    EXPECT_EQ(output.height(), 90);
}

TEST(BlackBarProcess, CropsBottomBars)
{
    BlackBarProcess filter;
    QImage input = imageWithBars(0, 10, Qt::red);
    QImage output = filter.process(input);
    EXPECT_EQ(output.height(), 90);
}

TEST(BlackBarProcess, CropsBothBars)
{
    BlackBarProcess filter;
    QImage input = imageWithBars(10, 10, Qt::red);
    QImage output = filter.process(input);
    EXPECT_EQ(output.height(), 80);
}

TEST(BlackBarProcess, WidthNeverChanges)
{
    BlackBarProcess filter;
    QImage input = imageWithBars(10, 10, Qt::blue);
    QImage output = filter.process(input);
    EXPECT_EQ(output.width(), 200);
}

TEST(BlackBarProcess, ContentPixelsPreserved)
{
    BlackBarProcess filter;
    QImage input = imageWithBars(10, 10, Qt::red);
    QImage output = filter.process(input);
    // First row of output should be the first content row (red)
    QRgb pixel = output.pixel(0, 0);
    EXPECT_GT(qRed(pixel),  200);
    EXPECT_LT(qGreen(pixel), 50);
    EXPECT_LT(qBlue(pixel),  50);
}
