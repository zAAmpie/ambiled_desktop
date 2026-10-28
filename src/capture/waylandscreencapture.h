#pragma once

#include "screencapture.h"
#include "perfstats.h"
class WaylandScreenCapture;

#ifdef Q_OS_UNIX
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMutex>
#include <atomic>

class QTimer;

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

//Hack to undefine some standard definitions from X11 messing with other libraries
#undef Bool
#undef Void

class WaylandScreenCapture : public QObject, public ScreenCapture
{
    /* Screen capture for Wayland sessions, via org.freedesktop.portal.ScreenCast + PipeWire.
     * Unlike the other backends this is asynchronous: the PipeWire thread only holds on to the
     * newest buffer the compositor delivered, and capture() copies (downsampled) out of it on
     * demand - so frames are only ever copied at our capture rate, not the compositor's, and
     * not at all if nothing new arrived since the last capture.
     * If the stream or portal session breaks (or the system resumes from suspend), it reconnects
     * itself with backoff, reusing the restore_token so no share dialog is shown. */
    Q_OBJECT
public:
    //restoreToken: a previously-granted portal restore_token (see QSettings), or empty on first run
    explicit WaylandScreenCapture(QString restoreToken = QString());
    ~WaylandScreenCapture();

    //Get methods
    CaptureMode type() const override {return ScreenCapture::WaylandMode;}

    //Main function to start capturing the screen - returns the latest frame received from PipeWire,
    //downsampled by DownsampleFactor in each dimension
    CaptureValue capture() override;

    //The shared monitor, as reported by the portal (empty if it didn't say)
    QRect outputGeometry() const override;

signals:
    //The portal issued a new (or rotated) restore_token - persist it so future sessions skip the share dialog
    void restoreTokenUpdated(QString token);

private slots:
    //Portal D-Bus Request.Response handlers
    void onCreateSessionResponse(uint response, QVariantMap results);
    void onSelectSourcesResponse(uint response, QVariantMap results);
    void onStartResponse(uint response, QVariantMap results);
    //Portal Session.Closed handler - compositor/user revoked sharing mid-session
    void onSessionClosed();
    //logind Manager.PrepareForSleep handler - reconnect after resume, since the stream may not survive suspend
    void onPrepareForSleep(bool sleeping);

private:
    //Kick off negotiation: check the portal can share monitors at all, then createSession()
    void startNegotiation();
    //The CreateSession -> SelectSources -> Start -> OpenPipeWireRemote sequence
    void createSession();
    //Subscribe to a Request object's Response signal before issuing the call that creates it
    QString subscribeToRequest(const char *slot);
    //Negotiation/setup failed. While reconnecting this retries with backoff; otherwise (first attempt,
    //e.g. portal unavailable) or if the user denied permission, gives up until the capture mode is re-selected
    void fail(const QString &reason);
    //Reconnect after a delay that backs off with consecutive attempts (reset once frames flow again)
    void scheduleRestart(const QString &reason);
    //Tear everything down and negotiate a fresh portal session + stream
    void restart();
    //Close the portal session (if any), ignoring the Closed signal that may follow
    void closeSession();
    //pLastError is read from the capture thread - always set it through here
    void setLastError(const QString &error);

    //PipeWire setup/teardown, run once we have a node id and a pre-authenticated socket fd
    void setupPipeWire(int pipewireFd, quint32 nodeId);
    void teardownPipeWire();
    static void onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *param);
    static void onStreamProcess(void *data);
    static void onStreamRemoveBuffer(void *data, struct pw_buffer *buffer);
    static void onStreamStateChanged(void *data, enum pw_stream_state oldState, enum pw_stream_state newState, const char *error);

    //D-Bus session state
    QString pSessionHandle;
    QString pRestoreToken;
    bool pPermanentlyFailed;
    bool pUserDenied;
    QString pLastError; //Guarded by pFrameMutex

    //Reconnection
    QTimer *pRestartTimer;
    std::atomic<int> pRestartAttempts; //Consecutive attempts without a frame - reset by capture()

    //PipeWire state
    pw_thread_loop *pLoop;
    pw_context *pContext;
    pw_core *pCore;
    pw_stream *pStream;
    spa_hook pStreamListener;
    spa_video_info_raw pVideoInfo;
    std::atomic<bool> pFormatNegotiated; //Written from the PipeWire thread, read from capture()
    std::atomic<bool> pTearingDown;      //Set during teardownPipeWire(), so our own disconnect isn't treated as a failure

    //Newest buffer delivered by PipeWire, dequeued and held (not yet queued back) so capture() can
    //copy out of it. Swapped in from the PipeWire thread, read from capture() - both under pFrameMutex.
    mutable QMutex pFrameMutex;
    pw_buffer *pHeldBuffer;
    QRect pOutputGeometry; //Shared monitor from the portal's Start response - guarded by pFrameMutex
    quint64 pHeldSequence;   //Bumped each time a new buffer is held
    quint64 pFrameSequence;  //pHeldSequence that pFrame was last copied from

    //Only touched from the PipeWire thread
    PerfStats pPipeWireFrameStats;
};
#endif
