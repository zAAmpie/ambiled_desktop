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
  - Linux: X11 (via XShm shared-memory capture)
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
([execthread.h](execthread.h)):

```
ScreenCapture (per-platform) -> ScreenManager -> ProcessManager -> LEDS -> SerialManager
```

- **[screencapture.h](screencapture.h) / [screenmanager.h](screenmanager.h)** — `ScreenCapture`
  is the abstract base for grabbing a raw frame of the desktop. Each backend
  ([dxgiscreencapture](dxgiscreencapture.h), [dx11screencapture](dx11screencapture.h),
  [dx9screencapture](dx9screencapture.h), [gdiscreencapture](gdiscreencapture.h),
  [x11screencapture](x11screencapture.h)) implements it for one platform/API.
  `ScreenManager` owns the active backend, polls it on a timer at the configured frame rate,
  and emits `readyFrame` with the captured image.
- **[imageprocess.h](imageprocess.h) / [processmanager.h](processmanager.h)** — `ImageProcess`
  is the base class for an image-processing step
  ([blackbarprocess](blackbarprocess.h), [scaleprocess](scaleprocess.h) — which also covers
  linear/cubic scaling, [lowpassfilterprocess](lowpassfilterprocess.h)). `ProcessManager` runs
  the enabled steps in priority order, then applies an `ImageTransform` to reduce the result
  to edge strips.
- **[imagetransform.h](imagetransform.h)** — base class for turning an image into
  `ImageStrips` (one thin image per screen edge). Implementations:
  [simpletransform](simpletransform.h) (plain average), [striptransform](striptransform.h),
  [blockstriptransform](blockstriptransform.h), [blocktransform](blocktransform.h).
- **[leds.h](leds.h)** — `LEDS` holds the physical LED layout (`LEDPositions`), converts
  `ImageStrips` into a single 1-D LED image (`transformToLED`), and applies brightness and
  colour-temperature correction before handing it to the serial manager.
- **[serialmanager.h](serialmanager.h)** — frames and writes the LED image to the serial
  device, requests firmware version/lux readings, verifies parity/CRC, and automatically
  retries the connection once a second if the device is disconnected.
- **[ambiled.h](ambiled.h) / [ambiled.cpp](ambiled.cpp) / [ambiled.ui](ambiled.ui)** — the main
  window and system tray integration that wires the managers together, exposes their
  settings in the UI, and persists them with `QSettings`.
- **[types.h](types.h) / [util.h](util.h) / [pixmaputils.h](pixmaputils.h)** — shared value
  types (`ScreenSize`, `StripSizes`, `ReturnValue<T>` error-handling helper) and small image
  utility functions used across the pipeline.

## Building

This is a Qt widgets application built with `qmake`. It targets **Qt 6** (`QtCore`,
`QtGui`, `QtWidgets`, `QtSerialPort`).

### Linux

Install Qt 6 base, widgets and serial port modules, plus the X11 development libraries used
by the X11 capture backend (package names below are for Arch/CachyOS; adjust for your
distro):

```sh
sudo pacman -S qt6-base qt6-serialport libx11
```

Then build:

```sh
qmake6 ambiled_desktop.pro
make
```

### Windows

Requires Qt 6 with the SerialPort module, and the Windows SDK (for DXGI/Direct3D 9/11, GDI,
WIC and COM, used by the Windows capture backends). Open
`ambiled_desktop.sln`/`ambiled_desktop.vcxproj` in Visual Studio, or run:

```sh
qmake ambiled_desktop.pro
```

with the Qt-for-Windows `qmake` and build with `nmake`/`jom`.

The platform-specific capture backends and libraries are scoped in
[ambiled_desktop.pro](ambiled_desktop.pro) with `win32:`/`unix:`, so each platform only links
against the libraries it actually needs.

## License

MIT — see [LICENCE.txt](LICENCE.txt).
