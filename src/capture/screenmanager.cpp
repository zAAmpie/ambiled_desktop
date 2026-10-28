#include "screenmanager.h"
#include "dx11screencapture.h"
#include "dx9screencapture.h"
#include "dxgiscreencapture.h"
#include "gdiscreencapture.h"
#include <QTimer>
#ifdef Q_OS_UNIX
//Qt includes must come before the X11 ones below - X11/Xlib.h #defines a pile of generic names
//(None, KeyPress, Status, Bool, ...) that collide with Qt's own enum values (e.g. QEvent::Type)
//if any Qt header still needs fresh parsing after X11 pollutes the macro namespace.
#include <QGuiApplication>
#include "x11screencapture.h"
#include "waylandscreencapture.h"
#endif

#ifdef Q_OS_UNIX
namespace {
//QGuiApplication::platformName() reflects which QPA plugin Qt actually loaded, which is more
//reliable than checking the WAYLAND_DISPLAY env var - that can still be set even when the app's
//platform plugin is forced to xcb (e.g. for XShm-dependent tools like this one's X11Mode).
bool isWaylandSession()
{
    return QGuiApplication::platformName() == QLatin1String("wayland");
}
}
#endif

//Constructor
ScreenManager::ScreenManager(int frameRate, QString waylandRestoreToken, QObject *parent)
    : QObject(parent), pFrameRate(frameRate), pWaylandRestoreToken(std::move(waylandRestoreToken)), pTestPatternGenerationEnabled(false),
      pCaptureStats(QStringLiteral("capture"))
{
    Q_ASSERT(frameRate > 0);
    pFrameRate = std::clamp(frameRate, 1, 1000); //Just to be safe, clamp the frame rate

    //Set up screen capture
#ifdef Q_OS_WIN
    pScreen = std::make_unique<DXGIScreenCapture>();
#endif
#ifdef Q_OS_UNIX
    if (isWaylandSession())
    {
        auto *waylandCapture = new WaylandScreenCapture(pWaylandRestoreToken);
        connect(waylandCapture, &WaylandScreenCapture::restoreTokenUpdated, this, &ScreenManager::onWaylandCaptureRestoreTokenUpdated);
        pScreen = std::unique_ptr<ScreenCapture>(waylandCapture);
    }
    else
    {
        pScreen = std::unique_ptr<ScreenCapture>(new X11ScreenCapture());
    }
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
        pScreen = std::make_unique<GDIScreenCapture>();
        break;
    case ScreenCapture::DirectX9Mode:
        pScreen = std::make_unique<DX9ScreenCapture>();
        break;
    case ScreenCapture::DirectX11Mode:
        pScreen = std::make_unique<DX11ScreenCapture>();
        break;
    case ScreenCapture::DXGIMode:
        pScreen = std::make_unique<DXGIScreenCapture>();
        break;
#endif
#ifdef Q_OS_UNIX
    case ScreenCapture::X11Mode:
        pScreen = std::unique_ptr<ScreenCapture>(new X11ScreenCapture());
        break;
    case ScreenCapture::WaylandMode:
    {
        auto *waylandCapture = new WaylandScreenCapture(pWaylandRestoreToken);
        connect(waylandCapture, &WaylandScreenCapture::restoreTokenUpdated, this, &ScreenManager::onWaylandCaptureRestoreTokenUpdated);
        pScreen = std::unique_ptr<ScreenCapture>(waylandCapture);
        break;
    }
#endif
    default:
        break;
    }
    pMutex.unlock();
}

//The active WaylandScreenCapture reported a new/rotated restore_token - remember it (so the next
//reconstruction, e.g. from setCaptureMode(), uses it) and re-emit for AmbiLED to persist
void ScreenManager::onWaylandCaptureRestoreTokenUpdated(QString token)
{
    pWaylandRestoreToken = token;
    emit waylandRestoreTokenUpdated(token);
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

//Test pattern to render while test pattern mode is enabled (thread-safe)
void ScreenManager::setTestPattern(const TestPattern &pattern)
{
    QMutexLocker locker(&pMutex);
    pTestPattern = pattern;
    pTestPatternFrame = QImage();
}

//Test pattern generation mode (thread-safe) - patterns run at the normal frame rate, so animations
//and the low-pass filter behave as they would on captured frames
void ScreenManager::setTestPatternGeneration(bool state)
{
    {
        QMutexLocker locker(&pMutex);
        pTestPatternGenerationEnabled = state;
        if (state)
        {
            pTestPatternClock.start();
            pTestPatternFrame = QImage();
        }
    }
    startFrameTimer();
}

//Frame timer slot that is called n times per second
void ScreenManager::frameTimerElapsed()
{
    pMutex.lock(); //Lock here just in case we're trying to destroy the pScreen object while capturing
    if (pTestPatternGenerationEnabled)
    {
        //Update screen with test pattern
        QImage frame = testPatternFrame();
        pMutex.unlock();
        emit readyFrame(frame);
        return;
    }

    //Update screen as normal
    CaptureValue captureValue = [this] {PerfTimer timer(pCaptureStats); return pScreen->capture();}();
    pMutex.unlock();
    if (captureValue.success() && !captureValue.value.isNull())
        emit readyFrame(captureValue.value);
    else
        emit failed(captureValue.error);
}

//Start timer based on frame rate
void ScreenManager::startFrameTimer()
{
    //Start frame timer based on frame period
    int frameTime = qMax(1, 1000 / pFrameRate);
    pFrameTimer->start(frameTime);
}

//Renders the current test pattern frame - call with pMutex held. Sized like a captured 4K frame after
//the capture backends' downsampling. A static pattern is rendered once and then re-sent as the very
//same image, so processing can reuse its output instead of recomputing it every frame.
QImage ScreenManager::testPatternFrame()
{
    static const QSize FrameSize(3840 / ScreenCapture::DownsampleFactor, 2160 / ScreenCapture::DownsampleFactor);
    if (pTestPatternFrame.isNull() || pTestPattern.isAnimated())
        pTestPatternFrame = pTestPattern.render(FrameSize, pTestPatternClock.elapsed());
    return pTestPatternFrame;
}

