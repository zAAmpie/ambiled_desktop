#include <QtTest/QtTest>
#include <QImage>

#include "x11screencapture.h"

//Exercises X11ScreenCapture against whatever X server $DISPLAY points at (on a Wayland desktop,
//XWayland). It only reads the root window - nothing is shown on screen. Tests that need a display
//skip themselves when there isn't one.
class TstCapture : public QObject
{
    Q_OBJECT
private slots:
    void x11_destroyWithoutCaptureDoesNotCrash();
    void x11_failsCleanlyWithoutDisplay();
    void x11_capturesDownsampledFrame();
};

void TstCapture::x11_destroyWithoutCaptureDoesNotCrash()
{
    //Regression test: cleanup() used to call XDestroyImage etc. on null state, so destroying a
    //backend that never captured (e.g. when switching capture mode) crashed
    {
        X11ScreenCapture capture;
    }
    QVERIFY(true);
}

void TstCapture::x11_failsCleanlyWithoutDisplay()
{
    //Regression test: a failed XOpenDisplay wasn't checked, so the null display was dereferenced
    QByteArray savedDisplay = qgetenv("DISPLAY");
    qputenv("DISPLAY", ":4242"); //Nothing listens here
    {
        X11ScreenCapture capture;
        CaptureValue result = capture.capture();
        QVERIFY(result.failed());
        QVERIFY2(result.error.contains(QStringLiteral("Could not open X display")), qPrintable(result.error));
    } //...and cleans up the partial state on destruction
    qputenv("DISPLAY", savedDisplay);
}

void TstCapture::x11_capturesDownsampledFrame()
{
    //Regression test: the output frame was never allocated, so every capture returned a null image
    //(which the app treated as a failure)
    if (qEnvironmentVariableIsEmpty("DISPLAY"))
        QSKIP("No X display available");

    X11ScreenCapture capture;
    for (int i = 0; i < 2; ++i) //Second call reuses the initialised state
    {
        CaptureValue result = capture.capture();
        if (result.failed() && result.error.contains(QStringLiteral("Could not open X display")))
            QSKIP("X display not reachable");
        QVERIFY2(result.success(), qPrintable(result.error));
        QVERIFY(!result.value.isNull());
        QCOMPARE(result.value.size(), QSize(capture.size().width / ScreenCapture::DownsampleFactor,
                                            capture.size().height / ScreenCapture::DownsampleFactor));
    }
}

QTEST_GUILESS_MAIN(TstCapture)
#include "tst_capture.moc"
