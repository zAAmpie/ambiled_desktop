#include <QtTest/QtTest>
#include <QImage>
#include <algorithm>

#include "blackbarprocess.h"
#include "scaleprocess.h"
#include "lowpassfilterprocess.h"

namespace {

QImage makeSolidImage(int width, int height, QRgb colour)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(colour);
    return image;
}

}

class TstProcess : public QObject
{
    Q_OBJECT
private slots:
    void blackBarProcess_removesBars();
    void blackBarProcess_leavesUnbarredImageUnchanged();
    void scaleProcess_sampleScale();
    void scaleProcess_linearScale();
    void scaleProcess_cubicScale();
    void scaleProcess_percentageScale();
    void lowPassFilterProcess_firstFrameUnchanged();
    void lowPassFilterProcess_handlesRepeatedCalls();
};

void TstProcess::blackBarProcess_removesBars()
{
    const int width = 10;
    const int height = 50;
    const int barRows = 5;

    QImage image = makeSolidImage(width, height, qRgb(255, 0, 0));
    for (int y = 0; y < barRows; ++y)
    {
        QRgb *topLine = reinterpret_cast<QRgb *>(image.scanLine(y));
        QRgb *bottomLine = reinterpret_cast<QRgb *>(image.scanLine(height - 1 - y));
        std::fill(topLine, topLine + width, qRgb(0, 0, 0));
        std::fill(bottomLine, bottomLine + width, qRgb(0, 0, 0));
    }

    BlackBarProcess process;
    QImage result = process.process(image);

    QCOMPARE(result.width(), width);
    QCOMPARE(result.height(), height - 2 * barRows);
    for (int y = 0; y < result.height(); ++y)
        for (int x = 0; x < result.width(); ++x)
            QCOMPARE(result.pixel(x, y), qRgb(255, 0, 0));
}

void TstProcess::blackBarProcess_leavesUnbarredImageUnchanged()
{
    QImage image = makeSolidImage(10, 20, qRgb(0, 255, 0));
    BlackBarProcess process;
    QImage result = process.process(image);
    QCOMPARE(result.size(), image.size());
}

void TstProcess::scaleProcess_sampleScale()
{
    QImage image = makeSolidImage(40, 20, qRgb(10, 20, 30));
    ScaleProcess process(ImageProcess::SampleScale, ImageSize(10, 20));
    QImage result = process.process(image);
    QCOMPARE(result.width(), 20);
    QCOMPARE(result.height(), 10);
}

void TstProcess::scaleProcess_linearScale()
{
    //Regression test: this case previously used a bitwise-OR'd case label
    //(LinearScale | CubicScale) that only ever matched CubicScale, leaving
    //LinearScale to fall off the end of the function with no return value.
    QImage image = makeSolidImage(40, 20, qRgb(10, 20, 30));
    ScaleProcess process(ImageProcess::LinearScale, ImageSize(10, 20));
    QImage result = process.process(image);
    QCOMPARE(result.width(), 20);
    QCOMPARE(result.height(), 10);
}

void TstProcess::scaleProcess_cubicScale()
{
    QImage image = makeSolidImage(40, 20, qRgb(10, 20, 30));
    ScaleProcess process(ImageProcess::CubicScale, ImageSize(10, 20));
    QImage result = process.process(image);
    QCOMPARE(result.width(), 20);
    QCOMPARE(result.height(), 10);
}

void TstProcess::scaleProcess_percentageScale()
{
    QImage image = makeSolidImage(40, 20, qRgb(10, 20, 30));
    ScaleProcess process(ImageProcess::LinearScale, 0.5f);
    QImage result = process.process(image);
    QCOMPARE(result.width(), 20);
    QCOMPARE(result.height(), 10);
}

void TstProcess::lowPassFilterProcess_firstFrameUnchanged()
{
    //Regression test: the history queue used to be dereferenced via constFirst()
    //before anything had been enqueued, which is undefined behaviour on the very
    //first call.
    QImage image = makeSolidImage(16, 16, qRgb(123, 45, 67));
    LowPassFilterProcess process;
    QImage result = process.process(image);
    QCOMPARE(result.size(), image.size());
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            QCOMPARE(result.pixel(x, y), image.pixel(x, y));
}

void TstProcess::lowPassFilterProcess_handlesRepeatedCalls()
{
    LowPassFilterProcess process;
    for (int i = 0; i < 5; ++i)
    {
        QImage image = makeSolidImage(8, 8, qRgb(i * 10, i * 10, i * 10));
        QImage result = process.process(image);
        QCOMPARE(result.size(), image.size());
    }
}

QTEST_GUILESS_MAIN(TstProcess)
#include "tst_process.moc"
