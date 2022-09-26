#pragma once

#include <QObject>
#include "screen.h"
#include "processmanager.h"

class ScreenManager : public QObject
{
    /* Manages screen captures according to a specified rate and returns a specified size */
    Q_OBJECT
public:
    explicit ScreenManager(ScreenSize size, int frameRate, QObject *parent = nullptr);
    explicit ScreenManager(int frameRate, QObject *parent = nullptr);
    ~ScreenManager();

    //Current capture modes
    void setCaptureMode(Screen::CaptureMode mode);
    Screen::CaptureMode captureMode() const {return pScreen.type();}

    //Frame rate
    void setFrameRate(int rate);
    int frameRate() {return pFrameRate;}

    //Screen size
    void setScreenSize(ScreenSize size);
    ScreenSize screenSize() const {return pScreen.size();}
signals:
    //Signals that final processed image is ready
    void screenUpdated(QImage screenImage);


private:
    //Private slot that is called when processing has been completed
    void processCompleted(QImage processedImage);

    Screen pScreen;
    ProcessManager pProcessManager;

    int pFrameRate;
};

