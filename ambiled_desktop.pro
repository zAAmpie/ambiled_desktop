QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ambiled.cpp \
    blackbarprocess.cpp \
    blockstriptransform.cpp \
    blocktransform.cpp \
    dx11screencapture.cpp \
    dx9screencapture.cpp \
    dxgiscreencapture.cpp \
    execthread.cpp \
    gdiscreencapture.cpp \
    imageprocess.cpp \
    imagetransform.cpp \
    leds.cpp \
    lowpassfilterprocess.cpp \
    main.cpp \
    pixmaputils.cpp \
    processmanager.cpp \
    scaleprocess.cpp \
    screencapture.cpp \
    screenmanager.cpp \
    serialmanager.cpp \
    simpletransform.cpp \
    striptransform.cpp \
    util.cpp \
    x11screencapture.cpp

HEADERS += \
    ambiled.h \
    blackbarprocess.h \
    blockstriptransform.h \
    blocktransform.h \
    dx11screencapture.h \
    dx9screencapture.h \
    dxgiscreencapture.h \
    execthread.h \
    gdiscreencapture.h \
    imageprocess.h \
    imagetransform.h \
    leds.h \
    lowpassfilterprocess.h \
    pixmaputils.h \
    processmanager.h \
    scaleprocess.h \
    screencapture.h \
    screenmanager.h \
    serialmanager.h \
    simpletransform.h \
    striptransform.h \
    types.h \
    util.h \
    version.h \
    x11screencapture.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ambiled.qrc

FORMS += \
    ambiled.ui
