#pragma once

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include "screencapture.h"
#include "perfstats.h"
#include "testpattern.h"

#define DEFAULT_FRAMERATE 60

class ScreenManager : public QObject
{
    /* Manages screen captures according to a specified rate and returns a specified size */
    Q_OBJECT
public:
    //waylandRestoreToken: a previously-granted portal restore_token (see QSettings), passed in up
    //front so it's available before the default backend is even constructed - see AmbiLED's
    //constructor for why this can't just be set afterwards via a setter.
    ScreenManager(int frameRate = DEFAULT_FRAMERATE, QString waylandRestoreToken = QString(), QObject *parent = nullptr);
    ~ScreenManager();

    //Current capture modes (thread-safe)
    void setCaptureMode(ScreenCapture::CaptureMode mode);
    ScreenCapture::CaptureMode captureMode() const {return pScreen->type();}

    //Frame rate (thread-safe)
    void setFrameRate(int rate);
    int frameRate() {return pFrameRate;}

    //Screen size
    ScreenSize screenSize() const {return pScreen->size();}

    //Where the captured output sits on the desktop, if the backend knows (thread-safe) - see ScreenCapture::outputGeometry
    QRect capturedOutputGeometry();

    //Enable/disable capturing (thread-safe)
    void setEnabled(bool state);
    void enable() {setEnabled(true);}
    void disable() {setEnabled(false);}

    //Test pattern mode (thread-safe): while enabled, frames are rendered from the test pattern and fed
    //straight into processing instead of being captured from the screen
    void setTestPattern(const TestPattern &pattern);
    void setTestPatternGeneration(bool state);
    void enableTestPatternGeneration() {setTestPatternGeneration(true);}
    void disableTestPatternGeneration() {setTestPatternGeneration(false);}
signals:
    //Signals that final processed image is ready
    void readyFrame(QImage screenImage);

    //Signals failure
    void failed(QString message);

    //The Wayland portal issued a new (or rotated) restore_token - persist it via QSettings
    void waylandRestoreTokenUpdated(QString token);

private:
    //Frame timer slot that is called n times per second
    void frameTimerElapsed();

    //The active WaylandScreenCapture reported a new/rotated restore_token - remember it (so the
    //next reconstruction uses it) and re-emit for AmbiLED to persist
    void onWaylandCaptureRestoreTokenUpdated(QString token);

    //Start timer based on frame rate
    void startFrameTimer();

    //Renders the current test pattern frame - call with pMutex held
    QImage testPatternFrame();

    std::unique_ptr<ScreenCapture> pScreen;
    QTimer *pFrameTimer;

    QMutex pMutex;
    int pFrameRate;
    QString pWaylandRestoreToken;

    //Test pattern mode - guarded by pMutex
    bool pTestPatternGenerationEnabled;
    TestPattern pTestPattern;
    QElapsedTimer pTestPatternClock;
    QImage pTestPatternFrame; //Last rendered frame, re-sent as-is while the pattern is static

    PerfStats pCaptureStats;
};

