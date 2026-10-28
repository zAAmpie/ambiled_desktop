QT       += core gui serialport dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

win32: LIBS += -ld3d11 -ld3d9 -lgdi32 -lwindowscodecs -lole32

unix: LIBS += -lX11 -lXext
unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libpipewire-0.3

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    src/app \
    src/capture \
    src/process \
    src/transform \
    src/leds \
    src/serial \
    src/common \
    src/testpattern

SOURCES += \
    src/app/ambiled.cpp \
    src/app/main.cpp \
    src/app/parameterform.cpp \
    src/capture/dx11screencapture.cpp \
    src/capture/dx9screencapture.cpp \
    src/capture/dxgiscreencapture.cpp \
    src/capture/gdiscreencapture.cpp \
    src/capture/screencapture.cpp \
    src/capture/screenmanager.cpp \
    src/capture/waylandscreencapture.cpp \
    src/capture/x11screencapture.cpp \
    src/common/execthread.cpp \
    src/common/imageutils.cpp \
    src/common/perfstats.cpp \
    src/common/pixmaputils.cpp \
    src/common/util.cpp \
    src/leds/leds.cpp \
    src/process/blackbarprocess.cpp \
    src/process/imageprocess.cpp \
    src/process/lowpassfilterprocess.cpp \
    src/process/processmanager.cpp \
    src/process/scaleprocess.cpp \
    src/serial/serialmanager.cpp \
    src/testpattern/testpattern.cpp \
    src/transform/blockstriptransform.cpp \
    src/transform/blocktransform.cpp \
    src/transform/imagetransform.cpp \
    src/transform/simpletransform.cpp \
    src/transform/striptransform.cpp

HEADERS += \
    src/app/ambiled.h \
    src/app/parameterform.h \
    src/capture/dx11screencapture.h \
    src/capture/dx9screencapture.h \
    src/capture/dxgiscreencapture.h \
    src/capture/gdiscreencapture.h \
    src/capture/screencapture.h \
    src/capture/screenmanager.h \
    src/capture/waylandscreencapture.h \
    src/capture/x11screencapture.h \
    src/common/execthread.h \
    src/common/imageutils.h \
    src/common/parameterspec.h \
    src/common/perfstats.h \
    src/common/pixmaputils.h \
    src/common/types.h \
    src/common/util.h \
    src/common/version.h \
    src/leds/leds.h \
    src/process/blackbarprocess.h \
    src/process/imageprocess.h \
    src/process/lowpassfilterprocess.h \
    src/process/processmanager.h \
    src/process/scaleprocess.h \
    src/serial/serialmanager.h \
    src/testpattern/testpattern.h \
    src/transform/blockstriptransform.h \
    src/transform/blocktransform.h \
    src/transform/imagetransform.h \
    src/transform/simpletransform.h \
    src/transform/striptransform.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    src/app/ambiled.qrc

FORMS += \
    src/app/ambiled.ui
