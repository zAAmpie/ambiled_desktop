#include <QtTest/QtTest>
#include <QImage>
#include <algorithm>

#include "blackbarprocess.h"
#include "scaleprocess.h"
#include "lowpassfilterprocess.h"
#include "processmanager.h"
#include <QSignalSpy>

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
    void lowPassFilterProcess_smoothsSubsequentFrames();
    void blackBarProcess_parametersRoundTrip();
    void processManager_keepsOnlyLatestPendingFrame();
    void processManager_reusesOutputForUnchangedFrame();
    void processManager_doesNotReuseWithStatefulProcess();
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

void TstProcess::lowPassFilterProcess_smoothsSubsequentFrames()
{
    //Regression test for two bugs that together made the filter a complete no-op:
    //1) the history queue used to enqueue the current frame then immediately dequeue that
    //   same frame back out as "previous", so it always diffed a frame against itself
    //2) the decay exponent used the elapsed time in milliseconds where the formula expects
    //   seconds, making the blend factor ~1000x too extreme at realistic cutoffs
    //With both fixed, a very different second frame fed in right after the first (negligible
    //elapsed time, so the filter should barely move towards it) must NOT come out identical
    //to the raw second input - today it would, since the filter has no effect at all.
    LowPassFilterProcess process;

    QImage first = makeSolidImage(8, 8, qRgb(0, 0, 0));
    process.process(first);

    QImage second = makeSolidImage(8, 8, qRgb(255, 255, 255));
    QImage result = process.process(second);

    QVERIFY(result.pixel(0, 0) != second.pixel(0, 0));
}

void TstProcess::blackBarProcess_parametersRoundTrip()
{
    BlackBarProcess process;
    QVariantMap parameters = process.parameters();
    QCOMPARE(parameters.value(QStringLiteral("scanThresholdPercentage")).toInt(), 25);
    QCOMPARE(parameters.value(QStringLiteral("blackValueThreshold")).toInt(), 5);

    QVariantMap updated;
    updated.insert(QStringLiteral("scanThresholdPercentage"), 10);
    updated.insert(QStringLiteral("blackValueThreshold"), 20);
    process.setParameters(updated);

    parameters = process.parameters();
    QCOMPARE(parameters.value(QStringLiteral("scanThresholdPercentage")).toInt(), 10);
    QCOMPARE(parameters.value(QStringLiteral("blackValueThreshold")).toInt(), 20);
}

namespace {
const StripSizes ManagerSizes(4, 4, 3, 3);
}

void TstProcess::processManager_keepsOnlyLatestPendingFrame()
{
    ProcessManager manager;
    QSignalSpy spy(&manager, &ProcessManager::readyProcess);

    //Three frames arrive before the process thread (here: this thread's event loop) gets a turn
    manager.startProcess(makeSolidImage(40, 30, qRgb(255, 0, 0)), ManagerSizes);
    manager.startProcess(makeSolidImage(40, 30, qRgb(0, 255, 0)), ManagerSizes);
    manager.startProcess(makeSolidImage(40, 30, qRgb(0, 0, 255)), ManagerSizes);
    QCoreApplication::processEvents();

    QCOMPARE(spy.count(), 1);
    ImageStrips strips = spy.at(0).at(0).value<ImageStrips>();
    QCOMPARE(strips.topLine.pixel(0, 0), qRgb(0, 0, 255)); //Only the newest frame was processed
}

void TstProcess::processManager_reusesOutputForUnchangedFrame()
{
    ProcessManager manager;
    QSignalSpy spy(&manager, &ProcessManager::readyProcess);
    QImage frame = makeSolidImage(40, 30, qRgb(10, 20, 30));

    manager.startProcess(frame, ManagerSizes);
    QCoreApplication::processEvents();
    manager.startProcess(frame, ManagerSizes);
    QCoreApplication::processEvents();

    //Still emitted every time, but the second is the very same (shared) output, not a recomputation
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).value<ImageStrips>().topLine.cacheKey(), spy.at(0).at(0).value<ImageStrips>().topLine.cacheKey());

    //A changed configuration must force recomputation even for the same frame
    manager.setTransform(ImageTransform::SimpleTransform);
    manager.startProcess(frame, ManagerSizes);
    QCoreApplication::processEvents();
    QCOMPARE(spy.count(), 3);
    QVERIFY(spy.at(2).at(0).value<ImageStrips>().topLine.cacheKey() != spy.at(1).at(0).value<ImageStrips>().topLine.cacheKey());
}

void TstProcess::processManager_doesNotReuseWithStatefulProcess()
{
    ProcessManager manager;
    manager.enableProcess(ImageProcess::LowPassFilter);
    QSignalSpy spy(&manager, &ProcessManager::readyProcess);
    QImage frame = makeSolidImage(40, 30, qRgb(10, 20, 30));

    manager.startProcess(frame, ManagerSizes);
    QCoreApplication::processEvents();
    manager.startProcess(frame, ManagerSizes);
    QCoreApplication::processEvents();

    QCOMPARE(spy.count(), 2);
    QVERIFY(spy.at(1).at(0).value<ImageStrips>().topLine.cacheKey() != spy.at(0).at(0).value<ImageStrips>().topLine.cacheKey());
}

QTEST_GUILESS_MAIN(TstProcess)
#include "tst_process.moc"
