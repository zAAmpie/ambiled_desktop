#include "screenmanager.h"
#include "dx11screencapture.h"
#include "dx9screencapture.h"
#include "dxgiscreencapture.h"
#include "gdiscreencapture.h"
#include "x11screencapture.h"
#include <QTimer>
#include <QPainter>
#include <QConicalGradient>

//Constructor
ScreenManager::ScreenManager(int frameRate, QObject *parent) : QObject(parent), pFrameRate(frameRate)
{
    Q_ASSERT(frameRate > 0);
    pFrameRate = std::clamp(frameRate, 1, 1000); //Just to be safe, clamp the frame rate

    //Set up screen capture
#ifdef Q_OS_WIN
    pScreen = std::unique_ptr<ScreenCapture>(new DXGIScreenCapture());
#endif
#ifdef Q_OS_UNIX
    pScreen = std::unique_ptr<ScreenCapture>(new X11ScreenCapture());
#endif

    //Start frame timer based on frame period
    pFrameTimer = new QTimer();
    connect(pFrameTimer, &QTimer::timeout, this, &ScreenManager::frameTimerElapsed);
    pFrameTimer->setTimerType(Qt::PreciseTimer);
    startFrameTimer();
}

//Destructor
ScreenManager::~ScreenManager()
{
    pFrameTimer->deleteLater();
}

//Current capture modes (thread-safe)
void ScreenManager::setCaptureMode(ScreenCapture::CaptureMode mode)
{
    if (pScreen->type() == mode)
        //Early out - no change required
        return;

    pMutex.lock();
    switch (mode)
    {
#ifdef Q_OS_WIN
    case ScreenCapture::GDIMode:
        pScreen = std::unique_ptr<ScreenCapture>(new GDIScreenCapture());
        break;
    case ScreenCapture::DirectX9Mode:
        pScreen = std::unique_ptr<ScreenCapture>(new DX9ScreenCapture());
        break;
    case ScreenCapture::DirectX11Mode:
        pScreen = std::unique_ptr<ScreenCapture>(new DX11ScreenCapture());
        break;
    case ScreenCapture::DXGIMode:
        pScreen = std::unique_ptr<ScreenCapture>(new DXGIScreenCapture());
        break;
#endif
#ifdef Q_OS_UNIX
    case ScreenCapture::X11Mode:
        pScreen = std::unique_ptr<ScreenCapture>(new X11ScreenCapture());
        break;
#endif
    default:
        break;
    }
    pMutex.unlock();
}

//Frame rate (thread-safe)
void ScreenManager::setFrameRate(int rate)
{
    Q_ASSERT(rate > 0);
    pMutex.lock();
    pFrameRate = std::clamp(rate, 1, 1000); //Just to be safe, clamp the frame rate

    startFrameTimer();
    pMutex.unlock();
}

//Enable/disable capturing (thread-safe)
void ScreenManager::setEnabled(bool state)
{
    //If we want to enable the timer and it was disabled, start it
    if (state && !pFrameTimer->isActive())
        startFrameTimer();

    //If we want to disable it and it is running, stop it
    if (!state && pFrameTimer->isActive())
        pFrameTimer->stop();
}

//Test pattern generation mode
void ScreenManager::setTestPatternGeneration(bool state)
{
    pTestPatternGenerationEnabled = state;
    if (pTestPatternGenerationEnabled)
    {
        //Test enabled
        pFrameTimer->start(100); //Slow down timer to 10 fps
    }
    else
    {
        //Test disabled
        startFrameTimer(); //Set timer back to normal
    }

}

//Frame timer slot that is called n times per second
void ScreenManager::frameTimerElapsed()
{
    if (pTestPatternGenerationEnabled)
    {
        //Update screen with test pattern
        emit readyFrame(generateTestPattern());
    }
    else
    {
        //Update screen as normal
        pMutex.lock(); //Lock here just in case we're trying to destroy the pScreen object while capturing
        emit readyFrame(pScreen->capture());
        pMutex.unlock();
    }
}

//Start timer based on frame rate
void ScreenManager::startFrameTimer()
{
    //Start frame timer based on frame period
    int frameTime = qMax(1, 1000 / pFrameRate);
    pFrameTimer->start(frameTime);
}

//Generate test pattern
QImage ScreenManager::generateTestPattern()
{
    QImage testImage = QImage(320,180, QImage::Format_RGB32);
    QPainter painter(&testImage);

    //Generate gradient
    QConicalGradient gradient;
    //Red - Green - Blue
    gradient.setColorAt(0, Qt::red);
    gradient.setColorAt(0.33, Qt::green);
    gradient.setColorAt(0.66, Qt::blue);

    //Fill with gradient
    painter.fillRect(testImage.rect(), gradient);

    return testImage;
}

