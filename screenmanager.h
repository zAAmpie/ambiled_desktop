#pragma once

#include <QObject>
#include "screen.h"

class ScreenManager : public QObject
{
    /* Manages screen captures according to a specified rate and returns a specified size */
    Q_OBJECT
public:
    explicit ScreenManager(int frameRate, QObject *parent = nullptr);
    ~ScreenManager();

    //Current capture modes
    void setCaptureMode(Screen::CaptureMode mode);
    Screen::CaptureMode captureMode() const {return pScreen->type();}

    //Frame rate
    void setFrameRate(int rate);
    int frameRate() {return pFrameRate;}

    //Screen size
    ScreenSize screenSize() const {return pScreen->size();}
signals:
    //Signals that final processed image is ready
    void readyFrame(QImage screenImage);


private:
    //Private slot that is called when capturing has been completed
    void captureCompleted(QImage processedImage);

    std::unique_ptr<Screen> pScreen;
    std::unique_ptr<QTimer> pFrameTimer;

    int pFrameRate;
};

