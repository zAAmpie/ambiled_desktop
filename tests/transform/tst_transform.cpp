#include <QtTest/QtTest>
#include <QImage>
#include <algorithm>
#include <cmath>

#include "imagetransform.h"
#include "simpletransform.h"
#include "striptransform.h"
#include "blockstriptransform.h"
#include "blocktransform.h"

namespace {

const int ImgWidth = 100;
const int ImgHeight = 80;
//Deliberately not a round multiple of any transform's default 10% strip width, to catch off-by-one rounding
const StripSizes TestSizes(12, 12, 9, 9); // top, bottom, left, right

//Builds an image where a band of rows at the top or bottom is one colour and the rest is another
QImage makeRowBandImage(int bandRows, bool bandAtTop, QRgb bandColour, QRgb restColour)
{
    QImage image(ImgWidth, ImgHeight, QImage::Format_RGB32);
    image.fill(restColour);
    int startRow = bandAtTop ? 0 : ImgHeight - bandRows;
    for (int y = startRow; y < startRow + bandRows; ++y)
    {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        std::fill(line, line + ImgWidth, bandColour);
    }
    return image;
}

//Builds an image where a band of columns at the left or right is one colour and the rest is another
QImage makeColumnBandImage(int bandCols, bool bandAtLeft, QRgb bandColour, QRgb restColour)
{
    QImage image(ImgWidth, ImgHeight, QImage::Format_RGB32);
    image.fill(restColour);
    int startCol = bandAtLeft ? 0 : ImgWidth - bandCols;
    for (int y = 0; y < ImgHeight; ++y)
    {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = startCol; x < startCol + bandCols; ++x)
            line[x] = bandColour;
    }
    return image;
}

bool isUniform(const QImage &image, QRgb colour)
{
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixel(x, y) != colour)
                return false;
    return true;
}

}

class TstTransform : public QObject
{
    Q_OBJECT
private:
    //Shared contract check for any ImageTransform implementation: given an edge band of a
    //known solid colour, the corresponding output strip must be that same solid colour at
    //the requested size, regardless of each transform's own intermediate scaling strategy.
    void verifyEdges(ImageTransform &transform);

private slots:
    void simpleTransform();
    void stripTransform();
    void blockStripTransform();
    void blockTransform();
    void outermostRowAndColumnAreSampled();
};

void TstTransform::verifyEdges(ImageTransform &transform)
{
    const QRgb colourA = qRgb(255, 0, 0);
    const QRgb colourB = qRgb(0, 0, 255);
    //Generous band (30%) relative to the transforms' default 10% sampling width, so every
    //transform's edge sample stays entirely within the band regardless of rounding.
    const int rowBand = static_cast<int>(std::ceil(ImgHeight * 0.3));
    const int colBand = static_cast<int>(std::ceil(ImgWidth * 0.3));

    {
        QImage image = makeRowBandImage(rowBand, true, colourA, colourB);
        ImageStrips strips = transform.transform(image, TestSizes);
        QCOMPARE(strips.topLine.size(), QSize(TestSizes.top, 1));
        QVERIFY2(isUniform(strips.topLine, colourA), "top strip should be uniformly the top band colour");
    }
    {
        QImage image = makeRowBandImage(rowBand, false, colourA, colourB);
        ImageStrips strips = transform.transform(image, TestSizes);
        QCOMPARE(strips.bottomLine.size(), QSize(TestSizes.bottom, 1));
        QVERIFY2(isUniform(strips.bottomLine, colourA), "bottom strip should be uniformly the bottom band colour");
    }
    {
        QImage image = makeColumnBandImage(colBand, true, colourA, colourB);
        ImageStrips strips = transform.transform(image, TestSizes);
        QCOMPARE(strips.leftLine.size(), QSize(1, TestSizes.left));
        QVERIFY2(isUniform(strips.leftLine, colourA), "left strip should be uniformly the left band colour");
    }
    {
        QImage image = makeColumnBandImage(colBand, false, colourA, colourB);
        ImageStrips strips = transform.transform(image, TestSizes);
        QCOMPARE(strips.rightLine.size(), QSize(1, TestSizes.right));
        QVERIFY2(isUniform(strips.rightLine, colourA), "right strip should be uniformly the right band colour");
    }
}

void TstTransform::simpleTransform()
{
    SimpleTransform transform;
    QCOMPARE(transform.type(), ImageTransform::SimpleTransform);
    verifyEdges(transform);
}

void TstTransform::stripTransform()
{
    StripTransform transform;
    QCOMPARE(transform.type(), ImageTransform::StripTransform);
    verifyEdges(transform);
}

void TstTransform::blockStripTransform()
{
    BlockStripTransform transform;
    QCOMPARE(transform.type(), ImageTransform::BlockStripTransform);
    verifyEdges(transform);
}

void TstTransform::blockTransform()
{
    BlockTransform transform;
    QCOMPARE(transform.type(), ImageTransform::BlockTransform);
    verifyEdges(transform);
}

void TstTransform::outermostRowAndColumnAreSampled()
{
    //Regression test: the bottom and right strips used to be taken one pixel in from the edge,
    //skipping the last row/column. SimpleTransform samples exactly the outermost 1px, so a band
    //that's only 1px wide must still come through.
    const QRgb edgeColour = qRgb(255, 0, 0);
    const QRgb restColour = qRgb(0, 0, 255);
    SimpleTransform transform;

    ImageStrips bottom = transform.transform(makeRowBandImage(1, false, edgeColour, restColour), TestSizes);
    QVERIFY2(isUniform(bottom.bottomLine, edgeColour), "bottom strip should be the last row");

    ImageStrips right = transform.transform(makeColumnBandImage(1, false, edgeColour, restColour), TestSizes);
    QVERIFY2(isUniform(right.rightLine, edgeColour), "right strip should be the last column");
}

QTEST_GUILESS_MAIN(TstTransform)
#include "tst_transform.moc"
