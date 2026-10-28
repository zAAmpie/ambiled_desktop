#include "waylandscreencapture.h"

#ifdef Q_OS_UNIX
//types.h's #define NoError "" (pulled in transitively above via screencapture.h) collides with
//QDBusError::ErrorType::NoError below - this file never uses the macro, so just drop it.
#undef NoError
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QDBusMetaType>
#include <QDBusArgument>
#include <QUuid>
#include <QTimer>
#include <QDebug>
#include "imageutils.h"
#include <unistd.h>

namespace {

//Start()'s response "streams" field is a(ua{sv}) - an array of (PipeWire node id, properties).
//Mirrors how xdg-desktop-portal-kde itself marshals this field. Registering it lets Qt's
//generic QDBusArgument operator>> templates (for std::pair and sequential containers) correctly
//walk the nested array-of-struct when extracted via direct operator>>, as done in onStartResponse.
using PortalStream = std::pair<quint32, QVariantMap>;

void ensurePortalStreamTypeRegistered()
{
    static const bool registered = []
    {
        qDBusRegisterMetaType<PortalStream>();
        qDBusRegisterMetaType<QList<PortalStream>>();
        return true;
    }();
    (void)registered;
}

//PipeWire has no refcounted/idempotent init - make sure we only ever call it once per process
void ensurePipeWireInitialised()
{
    static const bool initialised = []
    {
        pw_init(nullptr, nullptr);
        return true;
    }();
    (void)initialised;
}

}

//Constructor
WaylandScreenCapture::WaylandScreenCapture(QString restoreToken)
    : QObject(), ScreenCapture(), pRestoreToken(std::move(restoreToken)), pPermanentlyFailed(false), pUserDenied(false),
      pLastError(QStringLiteral("WaylandScreenCapture: Waiting for screen-share permission")),
      pRestartAttempts(0),
      pLoop(nullptr), pContext(nullptr), pCore(nullptr), pStream(nullptr), pFormatNegotiated(false), pTearingDown(false),
      pHeldBuffer(nullptr), pHeldSequence(0), pFrameSequence(0),
      pPipeWireFrameStats(QStringLiteral("wayland.pw-frame"))
{
    ensurePortalStreamTypeRegistered();

    pRestartTimer = new QTimer(this);
    pRestartTimer->setSingleShot(true);
    connect(pRestartTimer, &QTimer::timeout, this, &WaylandScreenCapture::restart);

    //The PipeWire stream doesn't reliably survive suspend - sometimes it errors (handled in
    //onStreamStateChanged), sometimes it just silently stops delivering frames - so always reconnect on resume
    QDBusConnection::systemBus().connect(
        QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"),
        QStringLiteral("org.freedesktop.login1.Manager"), QStringLiteral("PrepareForSleep"),
        this, SLOT(onPrepareForSleep(bool)));

    startNegotiation();
}

//Destructor
WaylandScreenCapture::~WaylandScreenCapture()
{
    teardownPipeWire();
    closeSession();
}

//Main function to start capturing the screen - returns the latest frame received from PipeWire
CaptureValue WaylandScreenCapture::capture()
{
    QMutexLocker locker(&pFrameMutex);
    if (!pFormatNegotiated)
        return CaptureValue(pLastError, pFrame);

    //Nothing new since the last capture - hand back the same (implicitly shared) image without copying
    if (!pHeldBuffer || pHeldSequence == pFrameSequence)
        return CaptureValue(pFrame);

    const spa_data &data = pHeldBuffer->buffer->datas[0];
    const uchar *source = static_cast<const uchar *>(data.data) + data.chunk->offset;

    //BGRx/RGBx are both 32bpp with the byte order already matching one of Qt's native QImage
    //formats on a little-endian host - no manual channel shuffling needed, just the right format tag.
    QImage::Format format = (pVideoInfo.format == SPA_VIDEO_FORMAT_RGBx)
        ? QImage::Format_RGBX8888
        : QImage::Format_RGB32; //BGRx (and the default/fallback)

    pFrame = ImageUtils::downsample32(source, static_cast<int>(pVideoInfo.size.width), static_cast<int>(pVideoInfo.size.height),
                                      data.chunk->stride, format, DownsampleFactor);
    pFrameSequence = pHeldSequence;
    pRestartAttempts = 0; //Frames are flowing again - the next breakage starts from the shortest backoff
    return CaptureValue(pFrame);
}

//The shared monitor, as reported by the portal (empty if it didn't say)
QRect WaylandScreenCapture::outputGeometry() const
{
    QMutexLocker locker(&pFrameMutex);
    return pOutputGeometry;
}

//Subscribe to a Request object's Response signal before issuing the call that creates it, per the
//portal spec's escaping rule: strip the leading ':' from our unique bus name and replace '.' with '_'
QString WaylandScreenCapture::subscribeToRequest(const char *slot)
{
    QString handleToken = QStringLiteral("ambiled_%1").arg(QUuid::createUuid().toString(QUuid::Id128));

    QString sender = QDBusConnection::sessionBus().baseService();
    sender.remove(0, 1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    QString requestPath = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, handleToken);

    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.portal.Desktop"), requestPath,
        QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"),
        this, slot);

    return handleToken;
}

//Negotiation/setup failed - see header for when this retries and when it gives up
void WaylandScreenCapture::fail(const QString &reason)
{
    //While reconnecting (e.g. just after resume, when the portal or PipeWire may still be coming back
    //up), keep trying - unless the user explicitly said no, which must never re-prompt in a loop
    if (pRestartAttempts > 0 && !pUserDenied)
    {
        teardownPipeWire();
        scheduleRestart(reason);
        return;
    }

    //Otherwise give up; the user can switch capture modes away and back to force a fresh attempt
    qWarning() << reason;
    pPermanentlyFailed = true;
    setLastError(reason);
    teardownPipeWire();
}

//Reconnect after a backoff delay - no-op if one is already pending or we've given up
void WaylandScreenCapture::scheduleRestart(const QString &reason)
{
    if (pPermanentlyFailed || pRestartTimer->isActive())
        return;

    static const int backoffMs[] = {2000, 5000, 10000, 30000};
    int attempt = pRestartAttempts++;
    int delayMs = backoffMs[qMin(attempt, static_cast<int>(std::size(backoffMs)) - 1)];

    qWarning().noquote() << reason << QStringLiteral("- reconnecting in %1 s").arg(delayMs / 1000);
    pFormatNegotiated = false;
    setLastError(QStringLiteral("%1 - reconnecting").arg(reason));
    pRestartTimer->start(delayMs);
}

//Tear everything down and negotiate a fresh portal session + stream (silently, via the restore_token)
void WaylandScreenCapture::restart()
{
    teardownPipeWire();
    closeSession();
    startNegotiation();
}

//Close the portal session (if any), ignoring the Closed signal that may follow
void WaylandScreenCapture::closeSession()
{
    if (pSessionHandle.isEmpty())
        return;

    QDBusConnection::sessionBus().disconnect(
        QStringLiteral("org.freedesktop.portal.Desktop"), pSessionHandle,
        QStringLiteral("org.freedesktop.portal.Session"), QStringLiteral("Closed"),
        this, SLOT(onSessionClosed()));

    //Politely close the portal session rather than leaving it dangling until the bus connection drops
    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"), pSessionHandle,
        QStringLiteral("org.freedesktop.portal.Session"), QStringLiteral("Close"));
    QDBusConnection::sessionBus().asyncCall(message);
    pSessionHandle.clear();
}

//pLastError is read from the capture thread
void WaylandScreenCapture::setLastError(const QString &error)
{
    QMutexLocker locker(&pFrameMutex);
    pLastError = error;
}

//Kick off negotiation: check the portal can share monitors at all, then createSession()
void WaylandScreenCapture::startNegotiation()
{
    //A portal backend that can't screen-cast (e.g. KWin's screencast protocol unavailable to it)
    //still exposes the ScreenCast interface, but rejects CreateSession with an opaque "response 2".
    //Its AvailableSourceTypes property says so up front, so check that first for a useful error.
    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    message << QStringLiteral("org.freedesktop.portal.ScreenCast") << QStringLiteral("AvailableSourceTypes");

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *callWatcher)
    {
        QDBusPendingReply<QDBusVariant> reply = *callWatcher;
        callWatcher->deleteLater();

        //Bit 1 = MONITOR. If the property can't be read at all, carry on and let CreateSession report the problem.
        if (!reply.isError() && !(reply.value().variant().toUInt() & 1))
        {
            fail(QStringLiteral("WaylandScreenCapture: The desktop portal can't share screens (no monitor source available) - "
                                "check that screen recording works in your desktop, e.g. in Spectacle"));
            return;
        }
        createSession();
    });
}

//The CreateSession -> SelectSources -> Start -> OpenPipeWireRemote sequence
void WaylandScreenCapture::createSession()
{
    QString handleToken = subscribeToRequest(SLOT(onCreateSessionResponse(uint,QVariantMap)));
    QString sessionHandleToken = QStringLiteral("ambiled_session_%1").arg(QUuid::createUuid().toString(QUuid::Id128));

    QVariantMap options;
    options[QStringLiteral("handle_token")] = handleToken;
    options[QStringLiteral("session_handle_token")] = sessionHandleToken;

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.ScreenCast"), QStringLiteral("CreateSession"));
    message << options;

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *callWatcher)
    {
        if (callWatcher->isError())
            fail(QStringLiteral("WaylandScreenCapture: xdg-desktop-portal ScreenCast service unavailable (%1)").arg(callWatcher->error().message()));
        callWatcher->deleteLater();
    });
}

//CreateSession's Request.Response arrived
void WaylandScreenCapture::onCreateSessionResponse(uint response, QVariantMap results)
{
    if (pPermanentlyFailed)
        return;
    if (response != 0)
    {
        fail(QStringLiteral("WaylandScreenCapture: Failed to create portal session (response %1)").arg(response));
        return;
    }

    pSessionHandle = results.value(QStringLiteral("session_handle")).toString();
    if (pSessionHandle.isEmpty())
    {
        fail(QStringLiteral("WaylandScreenCapture: Portal did not return a session handle"));
        return;
    }

    //Watch for the compositor/user revoking the session later
    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.portal.Desktop"), pSessionHandle,
        QStringLiteral("org.freedesktop.portal.Session"), QStringLiteral("Closed"),
        this, SLOT(onSessionClosed()));

    QString handleToken = subscribeToRequest(SLOT(onSelectSourcesResponse(uint,QVariantMap)));
    QVariantMap options;
    options[QStringLiteral("handle_token")] = handleToken;
    options[QStringLiteral("types")] = quint32(1); //MONITOR
    options[QStringLiteral("multiple")] = false;
    options[QStringLiteral("cursor_mode")] = quint32(1); //Hidden - matches X11Mode's root-window grab, which has no software cursor baked in either
    options[QStringLiteral("persist_mode")] = quint32(2); //Persist until explicitly revoked, so the restore_token keeps working
    if (!pRestoreToken.isEmpty())
        options[QStringLiteral("restore_token")] = pRestoreToken;

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.ScreenCast"), QStringLiteral("SelectSources"));
    message << QVariant::fromValue(QDBusObjectPath(pSessionHandle)) << options;

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *callWatcher)
    {
        if (callWatcher->isError())
            fail(QStringLiteral("WaylandScreenCapture: SelectSources call failed (%1)").arg(callWatcher->error().message()));
        callWatcher->deleteLater();
    });
}

//SelectSources' Request.Response arrived
void WaylandScreenCapture::onSelectSourcesResponse(uint response, QVariantMap results)
{
    if (pPermanentlyFailed)
        return;
    if (response != 0)
    {
        fail(QStringLiteral("WaylandScreenCapture: Failed to select capture source (response %1)").arg(response));
        return;
    }

    //Different portal backends have been seen returning the rotated restore_token on either
    //SelectSources or Start's response - capture it defensively on both rather than assuming one
    QString token = results.value(QStringLiteral("restore_token")).toString();
    if (!token.isEmpty() && token != pRestoreToken)
    {
        pRestoreToken = token;
        emit restoreTokenUpdated(pRestoreToken);
    }

    QString handleToken = subscribeToRequest(SLOT(onStartResponse(uint,QVariantMap)));
    QVariantMap options;
    options[QStringLiteral("handle_token")] = handleToken;

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.ScreenCast"), QStringLiteral("Start"));
    //This is the call that pops the share-picker dialog, unless restore_token above was accepted
    message << QVariant::fromValue(QDBusObjectPath(pSessionHandle)) << QString() << options;

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *callWatcher)
    {
        if (callWatcher->isError())
            fail(QStringLiteral("WaylandScreenCapture: Start call failed (%1)").arg(callWatcher->error().message()));
        callWatcher->deleteLater();
    });
}

//Start's Request.Response arrived - response!=0 here means the user denied/cancelled the share dialog
void WaylandScreenCapture::onStartResponse(uint response, QVariantMap results)
{
    if (pPermanentlyFailed)
        return;
    if (response != 0)
    {
        pUserDenied = true;
        fail(QStringLiteral("WaylandScreenCapture: Screen share permission denied"));
        return;
    }

    QString token = results.value(QStringLiteral("restore_token")).toString();
    if (!token.isEmpty() && token != pRestoreToken)
    {
        pRestoreToken = token;
        emit restoreTokenUpdated(pRestoreToken);
    }

    //streams: a(ua{sv}) - array of (PipeWire node id, properties). We only asked for one (non-multiple).
    //Extract via direct operator>> (through the QDBusArgument), not QVariant::value<T>() - the
    //latter goes through QMetaType's runtime conversion machinery, which doesn't reliably invoke
    //the compile-time operator>> templates for a custom type like QList<std::pair<...>> the way
    //Qt's own qdbus CLI tool's qdbus_cast-based extraction does.
    QList<PortalStream> streams;
    qvariant_cast<QDBusArgument>(results.value(QStringLiteral("streams"))) >> streams;
    quint32 nodeId = streams.isEmpty() ? 0 : streams.first().first;

    if (nodeId == 0)
    {
        fail(QStringLiteral("WaylandScreenCapture: Portal did not return a PipeWire stream"));
        return;
    }

    //Optional per-stream "position" and "size" (both (ii), in the compositor's logical coordinates)
    //say which monitor was shared - used to show test patterns on the captured screen
    const QVariantMap streamProperties = streams.first().second;
    auto readPair = [&](const QString &key, int &first, int &second)
    {
        if (!streamProperties.contains(key))
            return false;
        const QDBusArgument argument = qvariant_cast<QDBusArgument>(streamProperties.value(key));
        argument.beginStructure();
        argument >> first >> second;
        argument.endStructure();
        return true;
    };
    int x = 0, y = 0, width = 0, height = 0;
    bool hasPosition = readPair(QStringLiteral("position"), x, y);
    bool hasSize = readPair(QStringLiteral("size"), width, height);
    {
        QMutexLocker locker(&pFrameMutex);
        pOutputGeometry = (hasPosition && hasSize) ? QRect(x, y, width, height) : QRect();
    }
    if (!hasPosition || !hasSize)
        qInfo() << "WaylandScreenCapture: Portal didn't report which monitor is shared (position/size missing)";

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.ScreenCast"), QStringLiteral("OpenPipeWireRemote"));
    message << QVariant::fromValue(QDBusObjectPath(pSessionHandle)) << QVariantMap();

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, nodeId](QDBusPendingCallWatcher *callWatcher)
    {
        QDBusPendingReply<QDBusUnixFileDescriptor> reply = *callWatcher;
        if (callWatcher->isError())
        {
            fail(QStringLiteral("WaylandScreenCapture: OpenPipeWireRemote call failed (%1)").arg(callWatcher->error().message()));
        }
        else
        {
            //dup() while the QDBusUnixFileDescriptor temporary is still alive - it closes its own
            //copy once this expression ends, so we need our own independent copy to hand to PipeWire
            int fd = ::dup(reply.value().fileDescriptor());
            if (fd < 0)
                fail(QStringLiteral("WaylandScreenCapture: Failed to duplicate PipeWire remote fd"));
            else
                setupPipeWire(fd, nodeId);
        }
        callWatcher->deleteLater();
    });
}

//Compositor closed the session mid-stream (e.g. around suspend, or a compositor restart) - reconnect
void WaylandScreenCapture::onSessionClosed()
{
    scheduleRestart(QStringLiteral("WaylandScreenCapture: Screen share session was closed"));
}

//Reconnect after resume, since the stream may not survive suspend. Only if a stream exists - if a
//negotiation is still in flight, restarting now would run a second one alongside it.
void WaylandScreenCapture::onPrepareForSleep(bool sleeping)
{
    if (!sleeping && pStream)
        scheduleRestart(QStringLiteral("WaylandScreenCapture: Resumed from suspend"));
}

//Set up the PipeWire stream once we have a pre-authenticated socket fd and a node id to connect to.
//Runs its own dedicated pw_thread_loop, independent of Qt's event loop, since PipeWire delivers
//frames at its own negotiated rate rather than on demand.
void WaylandScreenCapture::setupPipeWire(int pipewireFd, quint32 nodeId)
{
    ensurePipeWireInitialised();

    pLoop = pw_thread_loop_new("ambiled-waylandcapture", nullptr);
    if (!pLoop)
    {
        ::close(pipewireFd);
        fail(QStringLiteral("WaylandScreenCapture: Failed to create PipeWire loop"));
        return;
    }

    pContext = pw_context_new(pw_thread_loop_get_loop(pLoop), nullptr, 0);
    if (!pContext)
    {
        ::close(pipewireFd);
        fail(QStringLiteral("WaylandScreenCapture: Failed to create PipeWire context"));
        return;
    }

    if (pw_thread_loop_start(pLoop) < 0)
    {
        ::close(pipewireFd);
        fail(QStringLiteral("WaylandScreenCapture: Failed to start PipeWire loop thread"));
        return;
    }

    pw_thread_loop_lock(pLoop);

    pCore = pw_context_connect_fd(pContext, pipewireFd, nullptr, 0);
    if (!pCore)
    {
        pw_thread_loop_unlock(pLoop);
        fail(QStringLiteral("WaylandScreenCapture: Failed to connect to PipeWire"));
        return;
    }

    pw_properties *streamProps = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Screen",
        nullptr);
    pStream = pw_stream_new(pCore, "ambiled-screencast", streamProps);
    if (!pStream)
    {
        pw_thread_loop_unlock(pLoop);
        fail(QStringLiteral("WaylandScreenCapture: Failed to create PipeWire stream"));
        return;
    }

    static const pw_stream_events streamEvents = []
    {
        pw_stream_events events{};
        events.version = PW_VERSION_STREAM_EVENTS;
        events.param_changed = &WaylandScreenCapture::onStreamParamChanged;
        events.process = &WaylandScreenCapture::onStreamProcess;
        events.remove_buffer = &WaylandScreenCapture::onStreamRemoveBuffer;
        events.state_changed = &WaylandScreenCapture::onStreamStateChanged;
        return events;
    }();
    pw_stream_add_listener(pStream, &pStreamListener, &streamEvents, this);

    uint8_t buffer[1024];
    spa_pod_builder builder;
    spa_pod_builder_init(&builder, buffer, sizeof(buffer));

    //SPA_RECTANGLE/SPA_FRACTION expand to GNU compound literals, which C++ (unlike C) treats as
    //rvalues - the SPA_POD_CHOICE_RANGE_* macros below need addressable lvalues, hence these.
    spa_rectangle defaultSize = SPA_RECTANGLE(1920, 1080);
    spa_rectangle minSize = SPA_RECTANGLE(1, 1);
    spa_rectangle maxSize = SPA_RECTANGLE(8192, 4320);
    spa_fraction defaultFramerate = SPA_FRACTION(60, 1);
    spa_fraction minFramerate = SPA_FRACTION(0, 1);
    spa_fraction maxFramerate = SPA_FRACTION(1000, 1);

    const spa_pod *params[1];
    params[0] = reinterpret_cast<spa_pod *>(spa_pod_builder_add_object(&builder,
        SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(3, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_RGBx),
        SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&defaultSize, &minSize, &maxSize),
        SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&defaultFramerate, &minFramerate, &maxFramerate)));

    //Deprecated-but-correct here: the portal hands us an ephemeral numeric node id with no stable
    //name/serial we could target via the PW_KEY_TARGET_OBJECT property instead, so target_id is the
    //only way to connect to this specific stream.
    int result = pw_stream_connect(pStream, PW_DIRECTION_INPUT, nodeId,
        static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
        params, 1);

    pw_thread_loop_unlock(pLoop);

    if (result < 0)
    {
        fail(QStringLiteral("WaylandScreenCapture: Failed to connect PipeWire stream"));
        return;
    }
}

//Tear down PipeWire objects, in reverse order of creation, locking per PipeWire's threading rules
void WaylandScreenCapture::teardownPipeWire()
{
    pFormatNegotiated = false;
    pTearingDown = true;

    if (pLoop)
        pw_thread_loop_lock(pLoop);

    //Forget the held buffer before the stream (which owns it) goes away - taking the frame mutex
    //also waits out any capture() still copying from it
    {
        QMutexLocker locker(&pFrameMutex);
        pHeldBuffer = nullptr;
    }

    if (pStream)
    {
        pw_stream_destroy(pStream);
        pStream = nullptr;
    }
    if (pCore)
    {
        pw_core_disconnect(pCore);
        pCore = nullptr;
    }
    if (pContext)
    {
        pw_context_destroy(pContext);
        pContext = nullptr;
    }

    if (pLoop)
    {
        pw_thread_loop_unlock(pLoop);
        pw_thread_loop_stop(pLoop);
        pw_thread_loop_destroy(pLoop);
        pLoop = nullptr;
    }

    pTearingDown = false;
}

//Called on the PipeWire thread when the compositor picks a concrete format from our EnumFormat choice
void WaylandScreenCapture::onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *param)
{
    auto *self = static_cast<WaylandScreenCapture *>(data);
    if (id != SPA_PARAM_Format || !param)
        return;

    spa_video_info_raw videoInfo{};
    if (spa_format_video_raw_parse(param, &videoInfo) < 0)
        return;

    //capture() reads pVideoInfo while copying out of the held buffer, which belongs to the old format
    QMutexLocker locker(&self->pFrameMutex);
    self->pHeldBuffer = nullptr;
    self->pVideoInfo = videoInfo;
    self->pScreenSize = ScreenSize(static_cast<int>(videoInfo.size.height), static_cast<int>(videoInfo.size.width));
    self->pFormatNegotiated = true;
}

//Called on the PipeWire thread whenever a new frame buffer is ready to be read. Only swaps the newest
//buffer in as the held one - the actual (downsampled) copy happens on demand in capture().
void WaylandScreenCapture::onStreamProcess(void *data)
{
    auto *self = static_cast<WaylandScreenCapture *>(data);
    PerfTimer timer(self->pPipeWireFrameStats);

    //Drain the queue, keeping only the newest buffer and handing any older ones straight back
    pw_buffer *newest = nullptr;
    while (pw_buffer *buffer = pw_stream_dequeue_buffer(self->pStream))
    {
        if (newest)
            pw_stream_queue_buffer(self->pStream, newest);
        newest = buffer;
    }
    if (!newest)
        return;

    //Skip buffers without usable pixels (e.g. cursor-only metadata updates, corrupted frames, or anything
    //too small for the negotiated size - capture() trusts these bounds when copying)
    const spa_buffer *spaBuffer = newest->buffer;
    const spa_data *spaData = spaBuffer->n_datas > 0 ? &spaBuffer->datas[0] : nullptr;
    qsizetype rowBytes = static_cast<qsizetype>(self->pVideoInfo.size.width) * 4;
    qsizetype requiredBytes = spaData ? spaData->chunk->stride * static_cast<qsizetype>(self->pVideoInfo.size.height - 1) + rowBytes : 0;
    if (!self->pFormatNegotiated || !spaData || !spaData->data || spaData->chunk->size == 0
        || (spaData->chunk->flags & SPA_CHUNK_FLAG_CORRUPTED) || spaData->chunk->stride < rowBytes
        || spaData->chunk->offset + requiredBytes > spaData->maxsize)
    {
        pw_stream_queue_buffer(self->pStream, newest);
        return;
    }

    pw_buffer *previous;
    {
        QMutexLocker locker(&self->pFrameMutex);
        previous = self->pHeldBuffer;
        self->pHeldBuffer = newest;
        ++self->pHeldSequence;
    }

    //Safe outside the lock - capture() only ever reads pHeldBuffer, which no longer points at it
    if (previous)
        pw_stream_queue_buffer(self->pStream, previous);
}

//Called on the PipeWire thread when a buffer is about to be freed (e.g. on renegotiation) - make sure
//we're not still holding on to it
void WaylandScreenCapture::onStreamRemoveBuffer(void *data, struct pw_buffer *buffer)
{
    auto *self = static_cast<WaylandScreenCapture *>(data);
    QMutexLocker locker(&self->pFrameMutex);
    if (self->pHeldBuffer == buffer)
        self->pHeldBuffer = nullptr;
}

//Called on the PipeWire thread when the stream's connection state changes
void WaylandScreenCapture::onStreamStateChanged(void *data, enum pw_stream_state oldState, enum pw_stream_state newState, const char *error)
{
    Q_UNUSED(oldState)
    auto *self = static_cast<WaylandScreenCapture *>(data);
    if ((newState == PW_STREAM_STATE_ERROR || newState == PW_STREAM_STATE_UNCONNECTED) && !self->pTearingDown)
    {
        self->pFormatNegotiated = false;
        QString reason = error
            ? QStringLiteral("WaylandScreenCapture: PipeWire stream error (%1)").arg(QString::fromUtf8(error))
            : QStringLiteral("WaylandScreenCapture: PipeWire stream disconnected");

        //Reconnecting tears down this PipeWire thread, so it has to happen on the object's own thread.
        //Dropped automatically if the object is destroyed before it runs.
        QMetaObject::invokeMethod(self, [self, reason] {self->scheduleRestart(reason);}, Qt::QueuedConnection);
    }
}

#endif
