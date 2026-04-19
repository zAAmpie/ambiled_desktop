#include "pipewire_screencapture.h"

#ifdef Q_OS_UNIX

#include <spa/param/buffers.h>
#include <spa/param/video/type-info.h>
#include <spa/pod/builder.h>
#include <spa/buffer/type-info.h>

// types.h defines NoError as "" which clashes with QDBusError::NoError
#undef NoError

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QDBusArgument>
#include <QRandomGenerator>
#include <QObject>

#include <unistd.h>
#include <cstring>

// ---------------------------------------------------------------------------
// PortalResponseHandler — receives a single xdg-desktop-portal Response
// signal then self-destructs. Defined here because it's an implementation
// detail and needs Q_OBJECT / moc processing (see #include *.moc at bottom).
// ---------------------------------------------------------------------------
class PortalResponseHandler : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<void(uint, QVariantMap)>;

    PortalResponseHandler(const QString &requestPath, Callback cb)
        : QObject(nullptr), mCallback(std::move(cb))
    {
        QDBusConnection::sessionBus().connect(
            {},
            requestPath,
            "org.freedesktop.portal.Request",
            "Response",
            this,
            SLOT(onResponse(uint, QVariantMap)));
    }

public slots:
    void onResponse(uint response, QVariantMap results)
    {
        mCallback(response, results);
        deleteLater();
    }

private:
    Callback mCallback;
};

// ---------------------------------------------------------------------------
// Portal helpers
// ---------------------------------------------------------------------------
static QString makeToken()
{
    return QString("ambiled_%1").arg(QRandomGenerator::global()->generate());
}

static QString makeRequestPath(const QString &token)
{
    QString sender = QDBusConnection::sessionBus().baseService();
    sender = sender.mid(1);       // strip leading ':'
    sender.replace('.', '_');
    return QString("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);
}

static QDBusInterface portalIface()
{
    return QDBusInterface(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        QDBusConnection::sessionBus());
}

// ---------------------------------------------------------------------------
// PipeWireScreenCapture
// ---------------------------------------------------------------------------
PipeWireScreenCapture::PipeWireScreenCapture() : ScreenCapture()
{
    std::memset(&mStreamHook, 0, sizeof(mStreamHook));
    std::memset(&mVideoInfo,  0, sizeof(mVideoInfo));
}

PipeWireScreenCapture::~PipeWireScreenCapture()
{
    *mAlive = false;
    closeStream();
}

CaptureValue PipeWireScreenCapture::capture()
{
    if (mState == State::Idle) {
        mState = State::PortalPending;
        createSession();
    }

    if (mState == State::Failed)
        return CaptureValue(mError, pFrame);

    if (mState == State::PortalPending)
        return CaptureValue("PipeWireScreenCapture: waiting for portal permission", pFrame);

    QMutexLocker lock(&mFrameMutex);
    if (mLatestFrame.isNull())
        return CaptureValue("PipeWireScreenCapture: no frame received yet", pFrame);

    pFrame      = mLatestFrame;
    pScreenSize = ScreenSize(pFrame.height(), pFrame.width());
    return CaptureValue(pFrame);
}

void PipeWireScreenCapture::createSession()
{
    QString sessionToken = makeToken();
    QString requestToken = makeToken();
    QString requestPath  = makeRequestPath(requestToken);

    auto alive = mAlive;
    new PortalResponseHandler(requestPath, [this, alive](uint response, QVariantMap results) {
        if (!*alive) return;
        if (response != 0) {
            mState = State::Failed;
            mError = "Portal: CreateSession was rejected or cancelled";
            return;
        }
        mSessionHandle = results.value("session_handle").toString();
        if (mSessionHandle.isEmpty()) {
            mState = State::Failed;
            mError = QString("Portal: session_handle missing or empty in CreateSession response (keys: %1)")
                         .arg(results.keys().join(", "));
            return;
        }
        selectSources(mSessionHandle);
    });

    QVariantMap options;
    options["handle_token"]          = requestToken;
    options["session_handle_token"]  = sessionToken;

    QDBusMessage reply = portalIface().call("CreateSession", options);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        mState = State::Failed;
        mError = QString("Portal: CreateSession failed: %1").arg(reply.errorMessage());
    }
}

void PipeWireScreenCapture::selectSources(const QString &sessionHandle)
{
    QString requestToken = makeToken();
    QString requestPath  = makeRequestPath(requestToken);

    auto alive = mAlive;
    new PortalResponseHandler(requestPath, [this, alive, sessionHandle](uint response, QVariantMap) {
        if (!*alive) return;
        if (response != 0) {
            mState = State::Failed;
            mError = "Portal: SelectSources was rejected or cancelled";
            return;
        }
        start(sessionHandle);
    });

    QVariantMap options;
    options["handle_token"] = requestToken;
    options["types"]        = uint(1);     // MONITOR
    options["multiple"]     = false;
    options["cursor_mode"]  = uint(2);     // EMBEDDED

    QDBusMessage reply = portalIface().call("SelectSources",
        QDBusObjectPath(sessionHandle), options);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        mState = State::Failed;
        mError = QString("Portal: SelectSources failed: %1").arg(reply.errorMessage());
    }
}

void PipeWireScreenCapture::start(const QString &sessionHandle)
{
    QString requestToken = makeToken();
    QString requestPath  = makeRequestPath(requestToken);

    auto alive = mAlive;
    new PortalResponseHandler(requestPath, [this, alive, sessionHandle](uint response, QVariantMap results) {
        if (!*alive) return;
        if (response != 0) {
            mState = State::Failed;
            mError = "Portal: Start was rejected or cancelled";
            return;
        }

        // Extract first stream node ID from a(ua{sv})
        uint32_t nodeId = 0;
        const QDBusArgument streams = results.value("streams").value<QDBusArgument>();
        streams.beginArray();
        if (!streams.atEnd()) {
            QVariantMap streamProps;
            streams.beginStructure();
            streams >> nodeId >> streamProps;
            streams.endStructure();
        }
        streams.endArray();

        // Get the PipeWire remote fd
        QDBusMessage fdReply = portalIface().call("OpenPipeWireRemote",
            QDBusObjectPath(sessionHandle), QVariantMap{});
        if (fdReply.type() != QDBusMessage::ReplyMessage) {
            mState = State::Failed;
            mError = QString("Portal: OpenPipeWireRemote failed: %1").arg(fdReply.errorMessage());
            return;
        }

        QDBusUnixFileDescriptor dbusFd =
            fdReply.arguments().at(0).value<QDBusUnixFileDescriptor>();
        int fd = dup(dbusFd.fileDescriptor());
        if (fd < 0) {
            mState = State::Failed;
            mError = "PipeWire: dup() of portal fd failed";
            return;
        }

        openStream(fd, nodeId);
    });

    QVariantMap options;
    options["handle_token"] = requestToken;

    QDBusMessage reply = portalIface().call("Start",
        QDBusObjectPath(sessionHandle), QString(), options);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        mState = State::Failed;
        mError = QString("Portal: Start failed: %1").arg(reply.errorMessage());
    }
}

void PipeWireScreenCapture::openStream(int fd, uint32_t nodeId)
{
    pw_init(nullptr, nullptr);

    mThreadLoop = pw_thread_loop_new("ambiled-capture", nullptr);
    if (!mThreadLoop) {
        mState = State::Failed;
        mError = "PipeWire: failed to create thread loop";
        close(fd);
        return;
    }

    mContext = pw_context_new(pw_thread_loop_get_loop(mThreadLoop), nullptr, 0);
    if (!mContext) {
        mState = State::Failed;
        mError = "PipeWire: failed to create context";
        close(fd);
        pw_thread_loop_destroy(mThreadLoop);
        mThreadLoop = nullptr;
        return;
    }

    pw_thread_loop_start(mThreadLoop);
    pw_thread_loop_lock(mThreadLoop);

    mCore = pw_context_connect_fd(mContext, fd, nullptr, 0);
    if (!mCore) {
        pw_thread_loop_unlock(mThreadLoop);
        mState = State::Failed;
        mError = "PipeWire: failed to connect to core";
        return;
    }

    auto *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE,     "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE,     "Screen",
        nullptr);

    mStream = pw_stream_new(mCore, "ambiled-screen-capture", props);
    if (!mStream) {
        pw_thread_loop_unlock(mThreadLoop);
        mState = State::Failed;
        mError = "PipeWire: failed to create stream";
        return;
    }

    // Must be static: pw_stream_add_listener stores a pointer to this struct,
    // so it must outlive the stream.
    static pw_stream_events events{};
    events.version       = PW_VERSION_STREAM_EVENTS;
    events.state_changed = onStreamStateChanged;
    events.param_changed = onStreamParamChanged;
    events.process       = onStreamProcess;
    pw_stream_add_listener(mStream, &mStreamHook, &events, this);

    const spa_rectangle sizeDefault = SPA_RECTANGLE(1920, 1080);
    const spa_rectangle sizeMin     = SPA_RECTANGLE(1, 1);
    const spa_rectangle sizeMax     = SPA_RECTANGLE(16384, 16384);
    const spa_fraction  fpsDefault  = SPA_FRACTION(60, 1);
    const spa_fraction  fpsMin      = SPA_FRACTION(0, 1);
    const spa_fraction  fpsMax      = SPA_FRACTION(360, 1);

    uint8_t buf[1024];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
    const spa_pod *params[1];
    params[0] = (spa_pod *)spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType,       SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype,    SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format,    SPA_POD_CHOICE_ENUM_Id(2,
            SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRA),
        SPA_FORMAT_VIDEO_size,      SPA_POD_CHOICE_RANGE_Rectangle(
            &sizeDefault, &sizeMin, &sizeMax),
        SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(
            &fpsDefault, &fpsMin, &fpsMax));

    pw_stream_connect(mStream,
        PW_DIRECTION_INPUT,
        nodeId,
        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
        params, 1);

    pw_thread_loop_unlock(mThreadLoop);
}

void PipeWireScreenCapture::closeStream()
{
    if (!mThreadLoop) return;

    pw_thread_loop_stop(mThreadLoop);

    if (mStream) {
        spa_hook_remove(&mStreamHook);
        pw_stream_destroy(mStream);
        mStream = nullptr;
    }
    if (mCore) {
        pw_core_disconnect(mCore);
        mCore = nullptr;
    }
    if (mContext) {
        pw_context_destroy(mContext);
        mContext = nullptr;
    }
    pw_thread_loop_destroy(mThreadLoop);
    mThreadLoop = nullptr;
}

// ---------------------------------------------------------------------------
// PipeWire stream callbacks (called on the PW thread)
// ---------------------------------------------------------------------------
void PipeWireScreenCapture::onStreamStateChanged(void *data,
    enum pw_stream_state /*old*/, enum pw_stream_state newState, const char *error)
{
    auto *self = static_cast<PipeWireScreenCapture *>(data);
    if (newState == PW_STREAM_STATE_ERROR) {
        self->mState = State::Failed;
        self->mError = error ? QString::fromUtf8(error) : "PipeWire: stream error";
    }
}

void PipeWireScreenCapture::onStreamParamChanged(void *data, uint32_t id,
    const struct spa_pod *param)
{
    auto *self = static_cast<PipeWireScreenCapture *>(data);
    if (!param || id != SPA_PARAM_Format)
        return;

    if (spa_format_video_raw_parse(param, &self->mVideoInfo) < 0)
        return;

    // Tell PipeWire we want plain memory buffers
    uint8_t buf[512];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
    const spa_pod *params[1];
    params[0] = (spa_pod *)spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_dataType,
            SPA_POD_CHOICE_FLAGS_Int(1 << SPA_DATA_MemPtr));
    pw_stream_update_params(self->mStream, params, 1);

    const auto fmt = (self->mVideoInfo.format == SPA_VIDEO_FORMAT_BGRA)
                     ? QImage::Format_ARGB32
                     : QImage::Format_RGB32;

    QMutexLocker lock(&self->mFrameMutex);
    self->mLatestFrame = QImage(
        static_cast<int>(self->mVideoInfo.size.width),
        static_cast<int>(self->mVideoInfo.size.height),
        fmt);

    self->mState = State::Streaming;
}

void PipeWireScreenCapture::onStreamProcess(void *data)
{
    auto *self = static_cast<PipeWireScreenCapture *>(data);

    pw_buffer *pwBuf = pw_stream_dequeue_buffer(self->mStream);
    if (!pwBuf) return;

    spa_buffer *spaBuf  = pwBuf->buffer;
    const void *pixels  = spaBuf->datas[0].data;
    const uint32_t stride = spaBuf->datas[0].chunk->stride;

    if (pixels && stride > 0) {
        QMutexLocker lock(&self->mFrameMutex);
        if (!self->mLatestFrame.isNull()) {
            const int h = self->mLatestFrame.height();
            for (int y = 0; y < h; ++y) {
                std::memcpy(
                    self->mLatestFrame.scanLine(y),
                    static_cast<const uint8_t *>(pixels) + y * stride,
                    self->mLatestFrame.bytesPerLine());
            }
        }
    }

    pw_stream_queue_buffer(self->mStream, pwBuf);
}

#include "pipewire_screencapture.moc"

#endif // Q_OS_UNIX