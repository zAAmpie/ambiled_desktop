#include "testpattern.h"
#include <QConicalGradient>
#include <QPainter>
#include <QtMath>

namespace {

//How far into the screen the edge chase block reaches, as a fraction of the screen's size in that
//direction - comfortably deeper than the transforms' default 10% sampling strips
constexpr qreal EdgeChaseDepth = 0.2;

ParameterSpec colourSpec(const QString &key, const QString &label, TestPattern::Colour defaultColour)
{
    ParameterSpec spec;
    spec.kind = ParameterSpec::Choice;
    spec.key = key;
    spec.label = label;
    spec.defaultValue = defaultColour;
    spec.choices = {{QStringLiteral("Red"), TestPattern::Red}, {QStringLiteral("Green"), TestPattern::Green},
                    {QStringLiteral("Blue"), TestPattern::Blue}, {QStringLiteral("White"), TestPattern::White},
                    {QStringLiteral("Black"), TestPattern::Black}};
    return spec;
}

ParameterSpec intSpec(const QString &key, const QString &label, int minimum, int maximum, int step, int defaultValue, const QString &suffix)
{
    ParameterSpec spec;
    spec.key = key;
    spec.label = label;
    spec.minimum = minimum;
    spec.maximum = maximum;
    spec.step = step;
    spec.defaultValue = defaultValue;
    spec.suffix = suffix;
    return spec;
}

}

//Constructor - starts from the default value of every parameter
TestPattern::TestPattern(Type type) : pType(type)
{
    for (const ParameterSpec &spec : parameterSpecs(type))
        pParameters.insert(spec.key, spec.defaultValue);
}

//Display name of a pattern type
QString TestPattern::name(Type type)
{
    switch (type)
    {
    case SolidColour: return QStringLiteral("Solid colour");
    case ColourWheel: return QStringLiteral("Spinning colour wheel");
    case Alternating: return QStringLiteral("Alternating colours");
    case EdgeChase: return QStringLiteral("Edge chase");
    }
    return QString();
}

//Whether the pattern changes over time
bool TestPattern::isAnimated() const
{
    switch (pType)
    {
    case SolidColour: return false;
    case ColourWheel: return intParameter(QStringLiteral("rpm")) != 0;
    case Alternating:
    case EdgeChase: return true;
    }
    return false;
}

//Generic configuration contract (see ParameterSpec)
QList<ParameterSpec> TestPattern::parameterSpecs(Type type)
{
    switch (type)
    {
    case SolidColour:
        return {colourSpec(QStringLiteral("colour"), QStringLiteral("Colour"), White),
                intSpec(QStringLiteral("level"), QStringLiteral("Level"), 0, 100, 5, 100, QStringLiteral("%"))};
    case ColourWheel:
        return {intSpec(QStringLiteral("rpm"), QStringLiteral("Speed"), 0, 120, 1, 10, QStringLiteral(" rpm"))};
    case Alternating:
        return {colourSpec(QStringLiteral("colourA"), QStringLiteral("First colour"), White),
                colourSpec(QStringLiteral("colourB"), QStringLiteral("Second colour"), Black),
                intSpec(QStringLiteral("intervalMs"), QStringLiteral("Switch every"), 100, 5000, 100, 1000, QStringLiteral(" ms"))};
    case EdgeChase:
        return {colourSpec(QStringLiteral("colour"), QStringLiteral("Colour"), White),
                intSpec(QStringLiteral("lapSeconds"), QStringLiteral("Time per lap"), 1, 60, 1, 10, QStringLiteral(" s")),
                intSpec(QStringLiteral("blockPercent"), QStringLiteral("Block length"), 1, 25, 1, 5, QStringLiteral("% of edge"))};
    }
    return {};
}

//Updates any parameters present - unknown keys are ignored
void TestPattern::setParameters(const QVariantMap &parameters)
{
    for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it)
        if (pParameters.contains(it.key()))
            pParameters.insert(it.key(), it.value());
}

//Draws the pattern over the whole of `area`, as it looks `elapsedMs` after it started
void TestPattern::render(QPainter &painter, const QRect &area, qint64 elapsedMs) const
{
    switch (pType)
    {
    case SolidColour:
    {
        QColor colour = colourParameter(QStringLiteral("colour"));
        int level = intParameter(QStringLiteral("level"));
        painter.fillRect(area, QColor(colour.red() * level / 100, colour.green() * level / 100, colour.blue() * level / 100));
        break;
    }
    case ColourWheel:
    {
        //Red - Yellow - Green - Cyan - Blue - Magenta - Red, rotated by the elapsed time
        qreal angle = std::fmod(static_cast<qreal>(elapsedMs) * intParameter(QStringLiteral("rpm")) * 360.0 / 60000.0, 360.0);
        QConicalGradient gradient(QRectF(area).center(), angle);
        gradient.setColorAt(0.0 / 6.0, Qt::red);
        gradient.setColorAt(1.0 / 6.0, Qt::yellow);
        gradient.setColorAt(2.0 / 6.0, Qt::green);
        gradient.setColorAt(3.0 / 6.0, Qt::cyan);
        gradient.setColorAt(4.0 / 6.0, Qt::blue);
        gradient.setColorAt(5.0 / 6.0, Qt::magenta);
        gradient.setColorAt(6.0 / 6.0, Qt::red);
        painter.fillRect(area, gradient);
        break;
    }
    case Alternating:
    {
        qint64 interval = qMax(1, intParameter(QStringLiteral("intervalMs")));
        bool first = (elapsedMs / interval) % 2 == 0;
        painter.fillRect(area, colourParameter(first ? QStringLiteral("colourA") : QStringLiteral("colourB")));
        break;
    }
    case EdgeChase:
        renderEdgeChase(painter, area, elapsedMs);
        break;
    }
}

//Convenience - renders into a new RGB32 image of the given size
QImage TestPattern::render(QSize size, qint64 elapsedMs) const
{
    QImage image(size, QImage::Format_RGB32);
    QPainter painter(&image);
    render(painter, image.rect(), elapsedMs);
    return image;
}

//A block travelling clockwise round the edge (as seen from the front), starting at the top-left
//corner, on black. Positions along the edge are measured as a distance along the perimeter:
//top (left to right), right (top to bottom), bottom (right to left), left (bottom to top).
void TestPattern::renderEdgeChase(QPainter &painter, const QRect &area, qint64 elapsedMs) const
{
    painter.fillRect(area, Qt::black);

    const qreal width = area.width();
    const qreal height = area.height();
    const qreal perimeter = 2.0 * (width + height);
    const qreal lapMs = qMax(1, intParameter(QStringLiteral("lapSeconds"))) * 1000.0;
    const qreal start = std::fmod(static_cast<qreal>(elapsedMs) / lapMs, 1.0) * perimeter;
    const qreal length = perimeter * intParameter(QStringLiteral("blockPercent")) / 100.0;
    const qreal depthX = width * EdgeChaseDepth;  //Into the screen from the left/right edges
    const qreal depthY = height * EdgeChaseDepth; //Into the screen from the top/bottom edges
    const QColor colour = colourParameter(QStringLiteral("colour"));

    //Fills the part of [from, to) (perimeter distances, within one lap) that lies on each side
    auto fillSpan = [&](qreal from, qreal to)
    {
        auto clip = [&](qreal sideStart, qreal sideEnd, auto toRect)
        {
            qreal a = qMax(from, sideStart);
            qreal b = qMin(to, sideEnd);
            if (a < b)
                painter.fillRect(toRect(a - sideStart, b - sideStart).translated(area.topLeft()), colour);
        };
        clip(0, width, [&](qreal a, qreal b) {return QRectF(a, 0, b - a, depthY);});
        clip(width, width + height, [&](qreal a, qreal b) {return QRectF(width - depthX, a, depthX, b - a);});
        clip(width + height, 2 * width + height, [&](qreal a, qreal b) {return QRectF(width - b, height - depthY, b - a, depthY);});
        clip(2 * width + height, perimeter, [&](qreal a, qreal b) {return QRectF(0, height - b, depthX, b - a);});
    };

    //The block may wrap past the top-left corner back to the start of the lap
    fillSpan(start, qMin(start + length, perimeter));
    if (start + length > perimeter)
        fillSpan(0, start + length - perimeter);
}

QColor TestPattern::colourParameter(const QString &key) const
{
    switch (static_cast<Colour>(intParameter(key)))
    {
    case Red: return QColor(255, 0, 0);
    case Green: return QColor(0, 255, 0);
    case Blue: return QColor(0, 0, 255);
    case White: return QColor(255, 255, 255);
    case Black: return QColor(0, 0, 0);
    }
    return QColor(0, 0, 0);
}

int TestPattern::intParameter(const QString &key) const
{
    return pParameters.value(key).toInt();
}
