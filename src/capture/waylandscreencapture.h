#pragma once

#include "screencapture.h"
class WaylandScreenCapture;

#ifdef Q_OS_UNIX
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMutex>

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

//Hack to undefine some standard definitions from X11 messing with other libraries
#undef Bool
#undef Void

class WaylandScreenCapture : public QObject, public ScreenCapture
{
    /* Screen capture for Wayland sessions, via org.freedesktop.portal.ScreenCast + PipeWire.
     * Unlike the other backends this is asynchronous - capture() never blocks, it just
     * returns whatever frame the PipeWire stream most recently delivered. */
    Q_OBJECT
public:
    //restoreToken: a previously-granted portal restore_token (see QSettings), or empty on first run
    explicit WaylandScreenCapture(QString restoreToken = QString());
    ~WaylandScreenCapture();

    //Get methods
    CaptureMode type() const override {return ScreenCapture::WaylandMode;}

    //Main function to start capturing the screen - returns the latest frame received from PipeWire
    CaptureValue capture() override;

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

private:
    //Kick off the CreateSession -> SelectSources -> Start -> OpenPipeWireRemote sequence
    void startNegotiation();
    //Subscribe to a Request object's Response signal before issuing the call that creates it
    QString subscribeToRequest(const char *slot);
    //Give up negotiating (permission denied / portal unavailable) - do not auto-retry
    void failPermanently(const QString &reason);

    //PipeWire setup/teardown, run once we have a node id and a pre-authenticated socket fd
    void setupPipeWire(int pipewireFd, quint32 nodeId);
    void teardownPipeWire();
    static void onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *param);
    static void onStreamProcess(void *data);
    static void onStreamStateChanged(void *data, enum pw_stream_state oldState, enum pw_stream_state newState, const char *error);

    //D-Bus session state
    QString pSessionHandle;
    QString pRestoreToken;
    bool pPermanentlyFailed;
    QString pLastError;

    //PipeWire state
    pw_thread_loop *pLoop;
    pw_context *pContext;
    pw_core *pCore;
    pw_stream *pStream;
    spa_hook pStreamListener;
    spa_video_info_raw pVideoInfo;
    bool pFormatNegotiated;

    //Latest decoded frame, written from the PipeWire thread and read from capture()
    QMutex pFrameMutex;
    QImage pLatestFrame;
};
#endif
