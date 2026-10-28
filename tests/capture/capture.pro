QT += testlib gui core
QT -= widgets

CONFIG += testcase console c++17
CONFIG -= app_bundle
TEMPLATE = app

LIBS += -lX11 -lXext

INCLUDEPATH += \
    ../../src/capture \
    ../../src/common

SOURCES += \
    tst_capture.cpp \
    ../../src/capture/screencapture.cpp \
    ../../src/capture/x11screencapture.cpp \
    ../../src/common/imageutils.cpp

HEADERS += \
    ../../src/capture/screencapture.h \
    ../../src/capture/x11screencapture.h \
    ../../src/common/imageutils.h \
    ../../src/common/types.h
