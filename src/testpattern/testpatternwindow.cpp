#include "testpatternwindow.h"
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

namespace {
//How long the hint stays up after the window is shown or the mouse moves, the last part fading out
constexpr int HintDurationMs = 5000;
constexpr int HintFadeMs = 500;
//Mouse "moves" shorter than this are ignored - compositors send move events for a pointer that
//hasn't actually moved (e.g. on re-entry or cursor changes), which would keep the hint up forever
constexpr qreal HintMouseThreshold = 4.0;
}

//Constructor
TestPatternWindow::TestPatternWindow() : QOpenGLWindow()
{
    setTitle(QStringLiteral("AmbiLED test pattern"));
    setFlag(Qt::FramelessWindowHint);
    setCursor(Qt::BlankCursor);

    //Keep animated patterns (and a showing/fading hint) moving: schedule the next frame as soon as
    //this one is on screen
    connect(this, &QOpenGLWindow::frameSwapped, this, [this]
    {
        if (pPattern.isAnimated() || hintOpacity() > 0)
            update();
    });
}

void TestPatternWindow::setPattern(const TestPattern &pattern)
{
    pPattern = pattern;
    update();
}

//Shows the window fullscreen on `screen` (moving it there if it's already shown elsewhere)
void TestPatternWindow::showOn(QScreen *screen)
{
    if (isVisible() && this->screen() == screen)
        return;

    //Re-showing is the most reliable way to move a fullscreen window between outputs on Wayland
    hide();
    setScreen(screen);
    setGeometry(screen->geometry());
    if (!pClock.isValid())
        pClock.start();
    showFullScreen();
    showHint();
}

void TestPatternWindow::paintGL()
{
    QPainter painter(this);
    pPattern.render(painter, QRect(QPoint(0, 0), size()), pClock.isValid() ? pClock.elapsed() : 0);
    qreal opacity = hintOpacity();
    if (opacity > 0)
        paintHint(painter, opacity);
}

//Shows the hint for a few seconds, then fades it out (frames keep coming while it's up - see constructor)
void TestPatternWindow::showHint()
{
    pHintClock.start();
    update();
}

//0 (gone) to 1 (fully shown)
qreal TestPatternWindow::hintOpacity() const
{
    if (!pHintClock.isValid())
        return 0;
    qint64 remainingMs = HintDurationMs - pHintClock.elapsed();
    return qBound<qreal>(0, static_cast<qreal>(remainingMs) / HintFadeMs, 1);
}

//The hint sits in the middle of the screen, which no LED strip samples, so it never affects the LEDs
void TestPatternWindow::paintHint(QPainter &painter, qreal opacity)
{
    painter.setOpacity(opacity);

    const QString text = tr("Esc to close  ·  Alt+Tab for the AmbiLED window");

    QFont font = painter.font();
    font.setPixelSize(qMax(14, height() / 40));
    painter.setFont(font);

    QRectF textRect = painter.fontMetrics().boundingRect(text);
    QRectF box = textRect.adjusted(-font.pixelSize(), -font.pixelSize() / 2, font.pixelSize(), font.pixelSize() / 2);
    box.moveCenter(QRectF(0, 0, width(), height()).center());

    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(box, font.pixelSize() / 2, font.pixelSize() / 2);
    painter.fillPath(path, QColor(0, 0, 0, 180));
    painter.setPen(Qt::white);
    painter.drawText(box, Qt::AlignCenter, text);
}

//Actually moving the mouse brings the hint back (or keeps it up)
void TestPatternWindow::mouseMoveEvent(QMouseEvent *event)
{
    QPointF position = event->position();
    if ((position - pLastMousePosition).manhattanLength() >= HintMouseThreshold)
    {
        pLastMousePosition = position;
        showHint();
    }
    QOpenGLWindow::mouseMoveEvent(event);
}

//Esc dismisses the pattern
void TestPatternWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        hide();
        emit dismissed();
        return;
    }
    QOpenGLWindow::keyPressEvent(event);
}

//Closed through the compositor (e.g. Alt+F4 or the task manager) - treat like Esc
bool TestPatternWindow::event(QEvent *event)
{
    if (event->type() == QEvent::Close)
        emit dismissed();
    return QOpenGLWindow::event(event);
}
