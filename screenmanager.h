#pragma once

#include <QObject>
#include <QMutex>
#include "screencapture.h"

class ScreenManager : public QObject
{
    /* Manages screen captures according to a specified rate and returns a specified size */
    Q_OBJECT
public:
    explicit ScreenManager(int frameRate, QObject *parent = nullptr);
    ~ScreenManager();

    //Current capture modes (thread-safe)
    void setCaptureMode(ScreenCapture::CaptureMode mode);
    ScreenCapture::CaptureMode captureMode() const {return pScreen->type();}

    //Frame rate (thread-safe)
    void setFrameRate(int rate);
    int frameRate() {return pFrameRate;}

    //Screen size
    ScreenSize screenSize() const {return pScreen->size();}

    //Enable/disable capturing (thread-safe)
    void setEnabled(bool state);
    void enable() {setEnabled(true);}
    void disable() {setEnabled(false);}
signals:
    //Signals that final processed image is ready
    void readyFrame(QImage screenImage);

    //Signals failure
    void failed(QString message);

private:
    //Private slot that is called when capturing has been completed
    void captureCompleted(QImage processedImage);

    std::unique_ptr<ScreenCapture> pScreen;
    std::unique_ptr<QTimer> pFrameTimer;

    QMutex pMutex;
    int pFrameRate;
};

