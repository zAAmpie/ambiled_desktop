#include "testpatternwindow.h"
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>

//Constructor
TestPatternWindow::TestPatternWindow() : QOpenGLWindow()
{
    setTitle(QStringLiteral("AmbiLED test pattern"));
    setFlag(Qt::FramelessWindowHint);
    setCursor(Qt::BlankCursor);

    //Keep animated patterns moving: schedule the next frame as soon as this one is on screen
    connect(this, &QOpenGLWindow::frameSwapped, this, [this]
    {
        if (pPattern.isAnimated())
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
}

void TestPatternWindow::paintGL()
{
    QPainter painter(this);
    pPattern.render(painter, QRect(QPoint(0, 0), size()), pClock.isValid() ? pClock.elapsed() : 0);
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
