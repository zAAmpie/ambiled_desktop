#include <QtTest/QtTest>
#include <QImage>
#include <vector>

#include "imageutils.h"

namespace {

//Raw 32bpp buffer with optional per-row padding, mimicking a foreign (e.g. PipeWire) buffer
struct RawFrame
{
    int width;
    int height;
    qsizetype stride;
    std::vector<quint32> pixels;

    RawFrame(int w, int h, int paddingPixels = 0, quint32 fill = 0)
        : width(w), height(h), stride(static_cast<qsizetype>(w + paddingPixels) * 4),
          pixels(static_cast<size_t>((w + paddingPixels) * h), fill) {}

    quint32 &at(int x, int y) {return pixels[static_cast<size_t>(y * (stride / 4) + x)];}
    const uchar *data() const {return reinterpret_cast<const uchar *>(pixels.data());}
};

}

class TstImageUtils : public QObject
{
    Q_OBJECT
private slots:
    void downsample32_sizeIsFlooredDivision();
    void downsample32_uniformColourIsPreserved();
    void downsample32_averagesEachChannelAcrossBlock();
    void downsample32_fullIntensityDoesNotOverflow();
    void downsample32_samplesMiddleRowOfBlock();
    void downsample32_respectsStridePadding();
    void downsample32_factorOneIsPlainCopy();
};

void TstImageUtils::downsample32_sizeIsFlooredDivision()
{
    RawFrame frame(3443, 1441);
    QImage output = ImageUtils::downsample32(frame.data(), frame.width, frame.height, frame.stride, QImage::Format_RGB32, 4);
    QCOMPARE(output.size(), QSize(860, 360));
    QCOMPARE(output.format(), QImage::Format_RGB32);
}

void TstImageUtils::downsample32_uniformColourIsPreserved()
{
    //Garbage in the 4th (x) byte must not leak into the output, which must be a valid RGB32 image
    RawFrame frame(64, 32, 0, 0x12a0b0c0);
    QImage output = ImageUtils::downsample32(frame.data(), frame.width, frame.height, frame.stride, QImage::Format_RGB32, 4);
    for (int y = 0; y < output.height(); ++y)
        for (int x = 0; x < output.width(); ++x)
            QCOMPARE(output.pixel(x, y), qRgb(0xa0, 0xb0, 0xc0));
}

void TstImageUtils::downsample32_averagesEachChannelAcrossBlock()
{
    RawFrame frame(4, 4);
    const quint32 row[4] = {qRgb(10, 200, 0), qRgb(20, 100, 255), qRgb(30, 0, 255), qRgb(40, 100, 255)};
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            frame.at(x, y) = row[x];

    QImage output = ImageUtils::downsample32(frame.data(), frame.width, frame.height, frame.stride, QImage::Format_RGB32, 4);
    QCOMPARE(output.size(), QSize(1, 1));
    //(10+20+30+40)/4 = 25, (200+100+0+100)/4 = 100, (0+255*3)/4 = 191 (truncated)
    QCOMPARE(output.pixel(0, 0), qRgb(25, 100, 191));
}

void TstImageUtils::downsample32_fullIntensityDoesNotOverflow()
{
    RawFrame frame(16, 16, 0, 0xffffffff);
    QImage output = ImageUtils::downsample32(frame.data(), frame.width, frame.height, frame.stride, QImage::Format_RGB32, 16);
    QCOMPARE(output.pixel(0, 0), qRgb(255, 255, 255));
}

void TstImageUtils::downsample32_samplesMiddleRowOfBlock()
{
    RawFrame frame(4, 4);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            frame.at(x, y) = qRgb(y * 10, 0, 0);

    QImage output = ImageUtils::downsample32(frame.data(), frame.width, frame.height, frame.stride, QImage::Format_RGB32, 4);
    QCOMPARE(qRed(output.pixel(0, 0)), 20); //Row 2 of rows 0-3
}

void TstImageUtils::downsample32_respectsStridePadding()
{
    //Padding pixels are bright; real pixels are dark - any padding read would show up in the output
    RawFrame frame(8, 8, 3, qRgb(255, 255, 255));
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            frame.at(x, y) = qRgb(0, 0, 0);

    QImage output = ImageUtils::downsample32(frame.data(), frame.width, frame.height, frame.stride, QImage::Format_RGB32, 2);
    QCOMPARE(output.size(), QSize(4, 4));
    for (int y = 0; y < output.height(); ++y)
        for (int x = 0; x < output.width(); ++x)
            QCOMPARE(output.pixel(x, y), qRgb(0, 0, 0));
}

void TstImageUtils::downsample32_factorOneIsPlainCopy()
{
    RawFrame frame(5, 3);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 5; ++x)
            frame.at(x, y) = qRgb(x * 40, y * 80, 7);

    QImage output = ImageUtils::downsample32(frame.data(), frame.width, frame.height, frame.stride, QImage::Format_RGB32, 1);
    QCOMPARE(output.size(), QSize(5, 3));
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 5; ++x)
            QCOMPARE(output.pixel(x, y), qRgb(x * 40, y * 80, 7));
}

QTEST_GUILESS_MAIN(TstImageUtils)
#include "tst_imageutils.moc"
