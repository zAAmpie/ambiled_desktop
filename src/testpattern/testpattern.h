#pragma once

#include <QColor>
#include <QImage>
#include <QVariantMap>
#include "parameterspec.h"

class QPainter;

class TestPattern
{
    /* A synthetic pattern for testing the LED output - e.g. solid colours for colour temperature,
     * a spinning colour wheel for blending, hard cuts for the low-pass filter's response, a bar or
     * block travelling round the edge for the LED layout, a colour per LED segment for checking the
     * LED counts, and letterboxed content for black bar removal. A plain value type that renders
     * purely from its parameters (plus the bottom LED layout) and the elapsed time, so it can be
     * copied freely between threads. */
public:
    enum Type {SolidColour, ColourWheel, Alternating, EdgeChase, BlockChase, Segments, Letterbox};
    static constexpr Type Types[] = {SolidColour, ColourWheel, Alternating, EdgeChase, BlockChase, Segments, Letterbox};
    //Colours offered by the Choice parameters
    enum Colour {Red, Green, Blue, White, Black};

    explicit TestPattern(Type type = SolidColour);

    Type type() const {return pType;}
    static QString name(Type type);
    //Whether the pattern changes over time, and so has to be re-rendered every frame
    bool isAnimated() const;

    //Generic configuration contract (see ParameterSpec) - defaults come from the specs
    static QList<ParameterSpec> parameterSpecs(Type type);
    QVariantMap parameters() const {return pParameters;}
    void setParameters(const QVariantMap &parameters);

    //LED counts along the bottom (left segment, gap, right segment), so Segments can split the bottom
    //edge where the LEDs do - not a user parameter, it mirrors the main tab's LED layout
    void setBottomLayout(int left, int gap, int right);

    //Draws the pattern over the whole of `area`, as it looks `elapsedMs` after it started
    void render(QPainter &painter, const QRect &area, qint64 elapsedMs) const;
    QImage render(QSize size, qint64 elapsedMs) const;

private:
    QColor colourParameter(const QString &key) const;
    int intParameter(const QString &key) const;

    void renderEdgeChase(QPainter &painter, const QRect &area, qint64 elapsedMs) const;
    void renderBlockChase(QPainter &painter, const QRect &area, qint64 elapsedMs) const;
    void renderSegments(QPainter &painter, const QRect &area) const;

    Type pType;
    QVariantMap pParameters;
    int pBottomLeft, pBottomGap, pBottomRight; //See setBottomLayout - defaults to even halves, no gap
};
