# AmbiLED Desktop

Desktop application that captures the screen in real time, reduces it down to a strip of
colours around the screen edge, and streams that strip over a serial connection to drive an
ambient ("Ambilight"-style) LED strip mounted behind a monitor or TV.

The app runs as a Qt system-tray application: pick a serial port and LED layout once, then
leave it running in the background while it continuously samples the screen and pushes
updated colours to the LED hardware.

## Features

- Multiple screen capture backends, picked automatically for the platform:
  - Windows: DXGI (preferred), DirectX 11, DirectX 9, GDI
  - Linux: Wayland (via the `org.freedesktop.portal.ScreenCast` portal + PipeWire, preferred
    when running under a Wayland session, and recovering automatically after suspend) or X11
    (via XShm shared-memory capture)
- Configurable capture frame rate
- Configurable LED strip layout (top/left/right/bottom-left/bottom-gap/bottom-right counts)
- A pipeline of image processing steps (black-bar removal, scaling, low-pass filtering) that
  can each be enabled/disabled and reordered by priority
- Several strategies for turning a screen edge into an LED strip (simple average, strip
  average, block, block-strip)
- Colour temperature correction (with built-in black-body/gaseous light-source presets) and
  brightness control, including automatic brightness from an ambient light (lux) sensor
  reading reported by the hardware over serial
- A simple framed serial protocol to the LED controller (show/firmware/read-bytes/lux
  commands, with parity or CRC checking) with automatic reconnect if the device disappears
- System tray icon with live connected/disconnected status, debug/visualise modes for
  previewing the captured and processed image, and persisted settings between runs

## Architecture

The app is organised as a small pipeline, each stage owned by a manager class and connected
by Qt signals/slots so capture and processing can run on their own threads
([src/common/execthread.h](src/common/execthread.h)):

```
ScreenCapture (per-platform) -> ScreenManager -> ProcessManager -> LEDS -> SerialManager
```

The source tree mirrors this pipeline:

- **[src/capture/](src/capture/)** — `ScreenCapture` ([screencapture.h](src/capture/screencapture.h))
  is the abstract base for grabbing a raw frame of the desktop. Each backend
  ([dxgiscreencapture](src/capture/dxgiscreencapture.h), [dx11screencapture](src/capture/dx11screencapture.h),
  [dx9screencapture](src/capture/dx9screencapture.h), [gdiscreencapture](src/capture/gdiscreencapture.h),
  [x11screencapture](src/capture/x11screencapture.h),
  [waylandscreencapture](src/capture/waylandscreencapture.h)) implements it for one
  platform/API. Unlike the others, `WaylandScreenCapture` is asynchronous: it negotiates a
  screen-share session over D-Bus with `org.freedesktop.portal.ScreenCast` (persisting the
  portal's `restore_token` via `QSettings` so the share dialog only appears once), then
  receives frames from a PipeWire video stream on its own thread. That thread only holds on to
  the newest buffer; `capture()` copies out of it on demand, downsampling 4× in each dimension
  while copying ([imageutils.h](src/common/imageutils.h)), so frames are only copied at the
  app's capture rate and not at all when nothing new arrived. If the stream or portal session
  breaks, or the system resumes from suspend, it reconnects itself with backoff, reusing the
  `restore_token` so no dialog is shown.
  [screenmanager.h](src/capture/screenmanager.h)'s `ScreenManager` owns the active backend,
  auto-selecting Wayland or X11 at startup based on `QGuiApplication::platformName()`, polls it
  on a timer at the configured frame rate, and emits `readyFrame` with the captured image.
- **[src/process/](src/process/)** — `ImageProcess` ([imageprocess.h](src/process/imageprocess.h))
  is the base class for an image-processing step
  ([blackbarprocess](src/process/blackbarprocess.h), [scaleprocess](src/process/scaleprocess.h) —
  which also covers linear/cubic scaling, [lowpassfilterprocess](src/process/lowpassfilterprocess.h)).
  Each step declares a `stage()`: before-transform steps (black-bar removal, scaling) run on the
  captured image, after-transform steps (the low-pass filter) run on the resulting LED values.
  [processmanager.h](src/process/processmanager.h)'s `ProcessManager` runs the enabled
  before-transform steps in priority order, applies an `ImageTransform` to reduce the result to
  edge strips, then runs the after-transform steps on all strips packed into one row. It only
  ever processes the latest frame: a frame that arrives while the previous one is still waiting
  replaces it rather than queueing behind it, and an unchanged frame reuses the previous output
  (unless a stateful step like the low-pass filter needs every tick).
- **[src/transform/](src/transform/)** — `ImageTransform` ([imagetransform.h](src/transform/imagetransform.h))
  is the base class for turning an image into `ImageStrips` (one thin image per screen edge).
  Implementations: [simpletransform](src/transform/simpletransform.h) (plain average),
  [striptransform](src/transform/striptransform.h), [blockstriptransform](src/transform/blockstriptransform.h),
  [blocktransform](src/transform/blocktransform.h).
- **[src/leds/leds.h](src/leds/leds.h)** — `LEDS` holds the physical LED layout
  (`LEDPositions`), converts `ImageStrips` into a single 1-D LED image (`transformToLED`), and
  applies brightness and colour-temperature correction before handing it to the serial
  manager.
- **[src/serial/serialmanager.h](src/serial/serialmanager.h)** — frames and writes the LED
  image to the serial device, requests firmware version/lux readings, verifies parity/CRC,
  and automatically retries the connection once a second if the device is disconnected.
- **[src/app/](src/app/)** — [ambiled.h](src/app/ambiled.h) / [ambiled.cpp](src/app/ambiled.cpp) /
  [ambiled.ui](src/app/ambiled.ui) hold the main window and system tray integration that wires
  the managers together, exposes their settings in the UI, and persists them with
  `QSettings`; [main.cpp](src/app/main.cpp) is the entry point.
- **[src/common/](src/common/)** — shared value types (`ScreenSize`, `StripSizes`,
  `ReturnValue<T>` error-handling helper in [types.h](src/common/types.h)) and small
  utilities ([util.h](src/common/util.h), [pixmaputils.h](src/common/pixmaputils.h),
  [imageutils.h](src/common/imageutils.h), [execthread.h](src/common/execthread.h)) used across
  the pipeline, plus the per-stage timing instrumentation in [perfstats.h](src/common/perfstats.h)
  (see [Measuring performance](#measuring-performance)).

## Building

This is a Qt widgets application built with `qmake`. It targets **Qt 6** (`QtCore`,
`QtGui`, `QtWidgets`, `QtSerialPort`).

### Linux

Install a compiler toolchain, Qt 6 base, widgets, serial port and D-Bus modules, the X11
development libraries used by the X11 capture backend, and PipeWire's development headers used
by the Wayland capture backend (package names below are for Arch/CachyOS; adjust for your
distro):

```sh
sudo pacman -S --needed base-devel qt6-base qt6-serialport libx11 libxext libpipewire
```

Then build:

```sh
mkdir -p build-linux && cd build-linux
qmake6 ../ambiled_desktop.pro
make -j$(nproc)
```

At runtime:

- The Wayland backend needs `xdg-desktop-portal` plus the backend for your desktop environment
  (`xdg-desktop-portal-kde`, `xdg-desktop-portal-gnome`, ...) installed and running, since the
  app talks to them over D-Bus. If screen sharing fails with "Failed to create portal session
  (response 2)", the portal backend can't screen-cast at all - check that e.g. Spectacle's
  screen recording works.
- Your user needs access to the serial device, which on Arch-based distros means being in the
  `uucp` group (`sudo usermod -aG uucp $USER`, then log out and back in).

Prefer building on the machine that will run the app. On distros with CPU-optimised
repositories (e.g. CachyOS's `znver4`/x86-64-v4 repos), glibc marks every binary linked
against it as needing that ISA level, so a binary built there refuses to start on an older CPU
with "CPU ISA level is lower than required", even though the app's own code is generic x86-64.

### Windows

Requires Qt 6 with the SerialPort module, and the Windows SDK (for DXGI/Direct3D 9/11, GDI,
WIC and COM, used by the Windows capture backends). Build with the Qt-for-Windows `qmake`:

```sh
qmake ambiled_desktop.pro
```

and then `nmake`/`jom`.

The platform-specific capture backends and libraries are scoped in
[ambiled_desktop.pro](ambiled_desktop.pro) with `win32:`/`unix:`, so each platform only links
against the libraries it actually needs.

## Testing

Unit tests live under [tests/](tests/), one `qmake` subproject per module under test, using
Qt's `QtTest` framework. They compile the real sources from `src/` directly (not a copy), so
they always test the same code the app ships.

```sh
qmake6 tests/tests.pro -o build-tests/Makefile
make -C build-tests
make -C build-tests check
```

`CONFIG += testcase` in each test's `.pro` file is what provides the `check` target: it runs
the compiled test binary and reports a non-zero exit code if anything fails, so `make check`
can be wired into CI.

Currently covered:

- **[tests/transform/](tests/transform/)** — for every `ImageTransform` implementation
  (simple/strip/block-strip/block), verifies each of the four output strips has the requested
  size and reflects the correct edge of the input image.
- **[tests/process/](tests/process/)** — `BlackBarProcess` (bar removal, the unbarred
  passthrough case, and that its zero-copy output outlives its input), `ScaleProcess` (all
  three interpolation modes, both the fixed-size and percentage constructors),
  `LowPassFilterProcess` (first-frame behaviour, smoothing, and converging both up and down),
  and `ProcessManager` (latest-frame-wins handoff, reusing output for an unchanged frame, and
  running after-transform steps on the strips).
- **[tests/common/](tests/common/)** — `ImageUtils::downsample32` (per-channel averaging,
  no overflow at full intensity, row sampling, stride padding).
- **[tests/capture/](tests/capture/)** (Unix only) — `X11ScreenCapture` against whatever X
  server `$DISPLAY` points at (XWayland on a Wayland desktop; it only reads the root window):
  produces correctly downsampled frames, fails cleanly without a display, and can be destroyed
  before its first capture. Skips itself when no X server is available.

## Measuring performance

Performance changes should be judged on measurements rather than assumptions. There are three
tools, from most isolated to most realistic:

**Benchmarks** — [tests/bench/](tests/bench/) is a `QBENCHMARK` suite running every transform
and process (and the capture downsampler) at 1080p, 1440p and 2160p on deterministic noise
frames. It's built with the tests but isn't a `testcase`, so `make check` stays fast. Run it
through the summary script for one line per case:

```sh
tests/bench/summarize.sh build-tests/bench/bench              # everything
tests/bench/summarize.sh build-tests/bench/bench pipelineDefault  # one case
```

Each line reports wall time *and* process-wide CPU time per iteration. The difference matters:
Qt 6 runs large smooth scales on its own thread pool, so wall time alone understates their
cost. Benchmarks reuse a frame that stays in CPU cache, so they understate memory-bound stages
(like the capture copy) compared to the live app.

**Per-stage timings in the running app** — [perfstats.h](src/common/perfstats.h) logs count,
rate, mean, p95 and max for each pipeline stage every 5 seconds: `capture`,
`wayland.pw-frame` (buffers delivered by PipeWire), each `process.*` step, `transform`,
`process.total`, `process.dropped`/`process.reused`, and the GUI-thread `gui.*` handlers. It
costs nothing unless enabled:

```sh
QT_LOGGING_RULES="ambiled.perf.debug=true" \
QT_MESSAGE_PATTERN="%{time h:mm:ss} %{category}: %{message}" \
./build-linux/ambiled_desktop 2>&1 | tee ~/ambiled-perf.log
```

**Per-thread CPU usage** — [tests/bench/measure_live.sh](tests/bench/measure_live.sh) samples
`/proc` for a running instance and prints each thread's CPU % over a window (100% = one core).
The capture and process threads are named `ambiled-capture` and `ambiled-process`:

```sh
tests/bench/measure_live.sh "video-fullscreen" 30 | tee -a ~/ambiled-perf-cpu.txt
```

For before/after comparisons, run the same scenarios each time (e.g. static desktop,
fullscreen video, letterboxed video with the low-pass filter on) at the same resolution and
frame rate, with the window hidden in the tray. The live preview has a significant cost of
its own when the window is visible.

## License

MIT — see [LICENCE.txt](LICENCE.txt).
