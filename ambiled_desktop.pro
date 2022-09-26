QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ambiled.cpp \
    blackbarprocess.cpp \
    dx11screen.cpp \
    dx9screen.cpp \
    dxgiscreen.cpp \
    execthread.cpp \
    filterprocess.cpp \
    gdiscreen.cpp \
    imageprocess.cpp \
    imagetransform.cpp \
    leds.cpp \
    main.cpp \
    processmanager.cpp \
    scaleprocess.cpp \
    screen.cpp \
    screenmanager.cpp \
    serial.cpp \
    util.cpp \
    x11screen.cpp

HEADERS += \
    ambiled.h \
    blackbarprocess.h \
    dx11screen.h \
    dx9screen.h \
    dxgiscreen.h \
    execthread.h \
    filterprocess.h \
    gdiscreen.h \
    imageprocess.h \
    imagetransform.h \
    leds.h \
    processmanager.h \
    scaleprocess.h \
    screen.h \
    screenmanager.h \
    serial.h \
    types.h \
    util.h \
    version.h \
    x11screen.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ambiled.qrc

FORMS += \
    ambiled.ui
