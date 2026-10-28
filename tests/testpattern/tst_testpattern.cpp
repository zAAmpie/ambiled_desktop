#include <QtTest/QtTest>
#include <QImage>

#include "testpattern.h"

namespace {

bool isUniform(const QImage &image, QRgb colour)
{
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixel(x, y) != colour)
                return false;
    return true;
}

}

class TstTestPattern : public QObject
{
    Q_OBJECT
private slots:
    void defaultsComeFromSpecs();
    void setParametersIgnoresUnknownKeys();
    void solidColour_fillsWithColourAtLevel();
    void isAnimated_onlyWhenPatternChangesOverTime();
    void colourWheel_rotatesOverTime();
    void alternating_switchesEveryInterval();
    void edgeChase_travelsClockwiseFromTopLeft();
    void edgeChase_wrapsPastTopLeftCorner();
};

void TstTestPattern::defaultsComeFromSpecs()
{
    for (TestPattern::Type type : TestPattern::Types)
    {
        TestPattern pattern(type);
        const QList<ParameterSpec> specs = TestPattern::parameterSpecs(type);
        QCOMPARE(pattern.parameters().size(), specs.size());
        for (const ParameterSpec &spec : specs)
            QCOMPARE(pattern.parameters().value(spec.key), spec.defaultValue);
    }
}

void TstTestPattern::setParametersIgnoresUnknownKeys()
{
    TestPattern pattern(TestPattern::SolidColour);
    pattern.setParameters({{QStringLiteral("level"), 40}, {QStringLiteral("bogus"), 1}});
    QCOMPARE(pattern.parameters().value(QStringLiteral("level")).toInt(), 40);
    QVERIFY(!pattern.parameters().contains(QStringLiteral("bogus")));
}

void TstTestPattern::solidColour_fillsWithColourAtLevel()
{
    TestPattern pattern(TestPattern::SolidColour);
    QVERIFY(isUniform(pattern.render(QSize(16, 9), 0), qRgb(255, 255, 255))); //Default: white, 100%

    pattern.setParameters({{QStringLiteral("colour"), TestPattern::Red}, {QStringLiteral("level"), 50}});
    QVERIFY(isUniform(pattern.render(QSize(16, 9), 0), qRgb(127, 0, 0)));

    pattern.setParameters({{QStringLiteral("colour"), TestPattern::Blue}, {QStringLiteral("level"), 0}});
    QVERIFY(isUniform(pattern.render(QSize(16, 9), 0), qRgb(0, 0, 0)));
}

void TstTestPattern::isAnimated_onlyWhenPatternChangesOverTime()
{
    QVERIFY(!TestPattern(TestPattern::SolidColour).isAnimated());
    QVERIFY(TestPattern(TestPattern::ColourWheel).isAnimated());
    QVERIFY(TestPattern(TestPattern::Alternating).isAnimated());
    QVERIFY(TestPattern(TestPattern::EdgeChase).isAnimated());

    TestPattern stillWheel(TestPattern::ColourWheel);
    stillWheel.setParameters({{QStringLiteral("rpm"), 0}});
    QVERIFY(!stillWheel.isAnimated());
}

void TstTestPattern::colourWheel_rotatesOverTime()
{
    TestPattern pattern(TestPattern::ColourWheel);
    pattern.setParameters({{QStringLiteral("rpm"), 60}}); //One turn per second
    const QPoint probe(190, 50); //Right of centre, on a 200x100 frame

    QRgb start = pattern.render(QSize(200, 100), 0).pixel(probe);
    QRgb quarterTurn = pattern.render(QSize(200, 100), 250).pixel(probe);
    QRgb fullTurn = pattern.render(QSize(200, 100), 1000).pixel(probe);

    QVERIFY(start != quarterTurn);
    QCOMPARE(fullTurn, start);
}

void TstTestPattern::alternating_switchesEveryInterval()
{
    TestPattern pattern(TestPattern::Alternating);
    pattern.setParameters({{QStringLiteral("colourA"), TestPattern::Green}, {QStringLiteral("colourB"), TestPattern::Blue},
                           {QStringLiteral("intervalMs"), 500}});

    QVERIFY(isUniform(pattern.render(QSize(8, 8), 0), qRgb(0, 255, 0)));
    QVERIFY(isUniform(pattern.render(QSize(8, 8), 499), qRgb(0, 255, 0)));
    QVERIFY(isUniform(pattern.render(QSize(8, 8), 500), qRgb(0, 0, 255)));
    QVERIFY(isUniform(pattern.render(QSize(8, 8), 1000), qRgb(0, 255, 0)));
}

void TstTestPattern::edgeChase_travelsClockwiseFromTopLeft()
{
    //200x100 frame: perimeter 600, so a 5% block is 30 long and a 10 s lap covers 60 per second
    TestPattern pattern(TestPattern::EdgeChase);
    pattern.setParameters({{QStringLiteral("lapSeconds"), 10}, {QStringLiteral("blockPercent"), 5}});
    const QRgb lit = qRgb(255, 255, 255);
    const QRgb dark = qRgb(0, 0, 0);

    //At the start the block sits at the top-left, running along the top edge
    QImage start = pattern.render(QSize(200, 100), 0);
    QCOMPARE(start.pixel(5, 5), lit);
    QCOMPARE(start.pixel(100, 50), dark); //Centre is never lit
    QCOMPARE(start.pixel(195, 60), dark);

    //Perimeter distance 250 (= 200 along the top + 50 down the right) is reached after 250/60 s
    QImage rightSide = pattern.render(QSize(200, 100), 4167);
    QCOMPARE(rightSide.pixel(195, 60), lit);
    QCOMPARE(rightSide.pixel(5, 5), dark);
}

void TstTestPattern::edgeChase_wrapsPastTopLeftCorner()
{
    //At perimeter distance 590 the 30-long block covers the last 10 of the left edge (its top) and
    //wraps round to the first 20 of the top edge
    TestPattern pattern(TestPattern::EdgeChase);
    pattern.setParameters({{QStringLiteral("lapSeconds"), 10}, {QStringLiteral("blockPercent"), 5}});

    QImage image = pattern.render(QSize(200, 100), 9834);
    QCOMPARE(image.pixel(15, 5), qRgb(255, 255, 255)); //Top edge, past the corner
    QCOMPARE(image.pixel(5, 5), qRgb(255, 255, 255));  //Left edge, just below the corner
    QCOMPARE(image.pixel(5, 50), qRgb(0, 0, 0));      //Further down the left edge
}

QTEST_GUILESS_MAIN(TstTestPattern)
#include "tst_testpattern.moc"
