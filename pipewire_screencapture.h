#pragma once

#include "screencapture.h"  // pulls in Qt, which defines Q_OS_UNIX

#ifdef Q_OS_UNIX

#include <QMutex>
#include <atomic>
#include <memory>

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

class PipeWireScreenCapture : public ScreenCapture
{
public:
    PipeWireScreenCapture();
    ~PipeWireScreenCapture() override;

    CaptureMode type() const override { return ScreenCapture::PipeWireMode; }
    CaptureValue capture() override;

private:
    void createSession();
    void selectSources(const QString &sessionHandle);
    void start(const QString &sessionHandle);
    void openStream(int fd, uint32_t nodeId);
    void closeStream();

    static void onStreamStateChanged(void *data, enum pw_stream_state old,
                                     enum pw_stream_state newState, const char *error);
    static void onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *param);
    static void onStreamProcess(void *data);

    QString mSessionHandle;

    pw_thread_loop *mThreadLoop = nullptr;
    pw_context     *mContext    = nullptr;
    pw_core        *mCore       = nullptr;
    pw_stream      *mStream     = nullptr;

    spa_hook           mStreamHook{};
    spa_video_info_raw mVideoInfo{};

    QMutex mFrameMutex;
    QImage mLatestFrame;

    enum class State { Idle, PortalPending, Streaming, Failed };
    std::atomic<State> mState{State::Idle};
    QString mError;

    std::shared_ptr<bool> mAlive{std::make_shared<bool>(true)};
};

#endif // Q_OS_UNIX