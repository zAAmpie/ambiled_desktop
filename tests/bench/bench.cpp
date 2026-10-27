#include <QtTest/QtTest>
#include <QImage>
#include <QRandomGenerator>
#include <algorithm>
#include <sys/resource.h>

#include "blackbarprocess.h"
#include "scaleprocess.h"
#include "lowpassfilterprocess.h"
#include "simpletransform.h"
#include "striptransform.h"
#include "blockstriptransform.h"
#include "blocktransform.h"
#include "imageutils.h"

namespace {

//Roughly a typical ambilight layout - the exact counts barely matter, cost is dominated by input size
const StripSizes BenchSizes(40, 40, 23, 23); // top, bottom, left, right

//Deterministic noise - every run (and every before/after comparison) sees identical pixels
QImage makeNoiseFrame(QSize size)
{
    QImage image(size, QImage::Format_RGB32);
    QRandomGenerator generator(42);
    for (int y = 0; y < image.height(); ++y)
    {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x)
            line[x] = 0xff000000 | (generator.generate() & 0x00ffffff);
    }
    return image;
}

//Noise frame with 2.39:1 style letterbox bars top and bottom, to exercise the black bar scan
QImage makeLetterboxedFrame(QSize size)
{
    QImage image = makeNoiseFrame(size);
    int barRows = size.height() / 8;
    for (int y = 0; y < barRows; ++y)
    {
        std::fill_n(reinterpret_cast<QRgb *>(image.scanLine(y)), size.width(), qRgb(0, 0, 0));
        std::fill_n(reinterpret_cast<QRgb *>(image.scanLine(size.height() - 1 - y)), size.width(), qRgb(0, 0, 0));
    }
    return image;
}

//Frames are expensive to generate at 4K - build each one once and share it across benchmarks
const QImage &cachedFrame(QSize size, bool letterboxed)
{
    static QHash<QString, QImage> cache;
    QString key = QStringLiteral("%1x%2%3").arg(size.width()).arg(size.height()).arg(letterboxed ? "lb" : "");
    auto it = cache.find(key);
    if (it == cache.end())
        it = cache.insert(key, letterboxed ? makeLetterboxedFrame(size) : makeNoiseFrame(size));
    return *it;
}

//QBENCHMARK reports wall time, but Qt 6 farms large scaled()/convert operations out to its own
//thread pool - so also report process-wide CPU time (all threads) per iteration, which is what
//actually matters for CPU utilisation
class CpuMeter
{
public:
    CpuMeter() : pStartUs(cpuTimeUs()), pIterations(0) {}
    ~CpuMeter()
    {
        if (pIterations > 0)
            qInfo().noquote() << QStringLiteral("cpu: %1 msecs per iteration").arg(static_cast<double>(cpuTimeUs() - pStartUs) / 1000.0 / pIterations, 0, 'g', 3);
    }
    void tick() {++pIterations;}

private:
    static qint64 cpuTimeUs()
    {
        rusage usage{};
        getrusage(RUSAGE_SELF, &usage);
        return (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000LL + usage.ru_utime.tv_usec + usage.ru_stime.tv_usec;
    }

    qint64 pStartUs;
    qint64 pIterations;
};

void addResolutionRows()
{
    QTest::addColumn<QSize>("size");
    QTest::newRow("1080p") << QSize(1920, 1080);
    QTest::newRow("1440p") << QSize(2560, 1440);
    QTest::newRow("2160p") << QSize(3840, 2160);
}

void benchTransform(ImageTransform &transform)
{
    QFETCH(QSize, size);
    const QImage &frame = cachedFrame(size, false);
    CpuMeter meter;
    QBENCHMARK
    {
        meter.tick();
        ImageStrips strips = transform.transform(frame, BenchSizes);
        Q_UNUSED(strips);
    }
}

void benchProcess(ImageProcess &process, bool letterboxed = false)
{
    QFETCH(QSize, size);
    const QImage &frame = cachedFrame(size, letterboxed);
    CpuMeter meter;
    QBENCHMARK
    {
        meter.tick();
        QImage output = process.process(frame);
        Q_UNUSED(output);
    }
}

}

class Bench : public QObject
{
    Q_OBJECT
private slots:
    void transformSimple_data() {addResolutionRows();}
    void transformSimple() {SimpleTransform transform; benchTransform(transform);}
    void transformStrip_data() {addResolutionRows();}
    void transformStrip() {StripTransform transform; benchTransform(transform);}
    void transformBlockStrip_data() {addResolutionRows();}
    void transformBlockStrip() {BlockStripTransform transform; benchTransform(transform);}
    void transformBlock_data() {addResolutionRows();}
    void transformBlock() {BlockTransform transform; benchTransform(transform);}

    void processBlackBar_data() {addResolutionRows();}
    void processBlackBar() {BlackBarProcess process; benchProcess(process);}
    void processBlackBarLetterboxed_data() {addResolutionRows();}
    void processBlackBarLetterboxed() {BlackBarProcess process; benchProcess(process, true);}
    void processSampleScale_data() {addResolutionRows();}
    void processSampleScale() {ScaleProcess process(ImageProcess::SampleScale, 0.5f); benchProcess(process);}
    void processLinearScale_data() {addResolutionRows();}
    void processLinearScale() {ScaleProcess process(ImageProcess::LinearScale, 0.5f); benchProcess(process);}
    //Runs after the transform, on all strips packed into one row - so its input doesn't depend on resolution
    void processLowPassFilter()
    {
        QImage strips = makeNoiseFrame(QSize(BenchSizes.top + BenchSizes.bottom + BenchSizes.left + BenchSizes.right, 1));
        LowPassFilterProcess process;
        CpuMeter meter;
        QBENCHMARK
        {
            meter.tick();
            QImage output = process.process(strips);
            Q_UNUSED(output);
        }
    }

    //Wayland capture's copy-out of the held PipeWire buffer (factor 4), vs the plain full-frame copy it replaced
    void captureDownsample4_data() {addResolutionRows();}
    void captureDownsample4()
    {
        QFETCH(QSize, size);
        const QImage &frame = cachedFrame(size, false);
        CpuMeter meter;
        QBENCHMARK
        {
            meter.tick();
            QImage output = ImageUtils::downsample32(frame.constBits(), frame.width(), frame.height(), frame.bytesPerLine(), QImage::Format_RGB32, 4);
            Q_UNUSED(output);
        }
    }
    void captureFullCopy_data() {addResolutionRows();}
    void captureFullCopy()
    {
        QFETCH(QSize, size);
        const QImage &frame = cachedFrame(size, false);
        CpuMeter meter;
        QBENCHMARK
        {
            meter.tick();
            QImage output = QImage(frame.constBits(), frame.width(), frame.height(), frame.bytesPerLine(), QImage::Format_RGB32).copy();
            Q_UNUSED(output);
        }
    }

    //The app's default pipeline: black bar removal followed by the default (block strip) transform
    void pipelineDefault_data() {addResolutionRows();}
    void pipelineDefault()
    {
        QFETCH(QSize, size);
        const QImage &frame = cachedFrame(size, true);
        BlackBarProcess blackBar;
        BlockStripTransform transform;
        CpuMeter meter;
        QBENCHMARK
        {
            meter.tick();
            ImageStrips strips = transform.transform(blackBar.process(frame), BenchSizes);
            Q_UNUSED(strips);
        }
    }
};

QTEST_GUILESS_MAIN(Bench)
#include "bench.moc"
