QT += testlib gui core
QT -= widgets

CONFIG += testcase console c++17
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += \
    ../../src/process \
    ../../src/common

SOURCES += \
    tst_process.cpp \
    ../../src/process/imageprocess.cpp \
    ../../src/process/blackbarprocess.cpp \
    ../../src/process/scaleprocess.cpp \
    ../../src/process/lowpassfilterprocess.cpp

HEADERS += \
    ../../src/process/imageprocess.h \
    ../../src/process/blackbarprocess.h \
    ../../src/process/scaleprocess.h \
    ../../src/process/lowpassfilterprocess.h \
    ../../src/common/types.h \
    ../../src/common/util.h
