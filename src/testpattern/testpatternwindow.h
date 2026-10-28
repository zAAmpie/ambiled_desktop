#pragma once

#include <QElapsedTimer>
#include <QOpenGLWindow>
#include "testpattern.h"

class QScreen;

class TestPatternWindow : public QOpenGLWindow
{
    /* Shows a TestPattern fullscreen on a given screen, so it's captured like any other content and
     * the whole chain (capture -> processing -> LEDs) is exercised. Painted through OpenGL, so even
     * an animated pattern at 4K costs the CPU very little and doesn't skew performance measurements.
     * Animated patterns redraw in step with the display; static ones only when changed. */
    Q_OBJECT
public:
    TestPatternWindow();

    void setPattern(const TestPattern &pattern);
    //Shows the window fullscreen on `screen` (moving it there if it's already shown elsewhere)
    void showOn(QScreen *screen);

signals:
    //The user dismissed the window (Esc, or closed it through the compositor)
    void dismissed();

protected:
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;

private:
    TestPattern pPattern;
    QElapsedTimer pClock;
};
