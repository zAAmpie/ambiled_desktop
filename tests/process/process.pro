QT += testlib gui core
QT -= widgets

CONFIG += testcase console c++17
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += \
    ../../src/process \
    ../../src/transform \
    ../../src/leds \
    ../../src/common \
    ../../src/testpattern

SOURCES += \
    tst_process.cpp \
    ../../src/testpattern/testpattern.cpp \
    ../../src/process/imageprocess.cpp \
    ../../src/process/blackbarprocess.cpp \
    ../../src/process/scaleprocess.cpp \
    ../../src/process/lowpassfilterprocess.cpp \
    ../../src/process/processmanager.cpp \
    ../../src/transform/imagetransform.cpp \
    ../../src/transform/simpletransform.cpp \
    ../../src/transform/striptransform.cpp \
    ../../src/transform/blockstriptransform.cpp \
    ../../src/transform/blocktransform.cpp \
    ../../src/common/perfstats.cpp

HEADERS += \
    ../../src/process/imageprocess.h \
    ../../src/process/blackbarprocess.h \
    ../../src/process/scaleprocess.h \
    ../../src/process/lowpassfilterprocess.h \
    ../../src/process/processmanager.h \
    ../../src/common/perfstats.h \
    ../../src/common/types.h \
    ../../src/common/util.h
