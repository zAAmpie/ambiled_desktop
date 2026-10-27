QT += testlib gui core
QT -= widgets

# Not a testcase - benchmarks are slow, so keep them out of "make check". Run explicitly:
#   ./bench                       (wall clock)
#   ./bench -callgrind            (instruction counts - deterministic, best for before/after)
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += \
    ../../src/process \
    ../../src/transform \
    ../../src/leds \
    ../../src/common

SOURCES += \
    bench.cpp \
    ../../src/process/imageprocess.cpp \
    ../../src/process/blackbarprocess.cpp \
    ../../src/process/scaleprocess.cpp \
    ../../src/process/lowpassfilterprocess.cpp \
    ../../src/transform/imagetransform.cpp \
    ../../src/transform/simpletransform.cpp \
    ../../src/transform/striptransform.cpp \
    ../../src/transform/blockstriptransform.cpp \
    ../../src/transform/blocktransform.cpp

HEADERS += \
    ../../src/process/imageprocess.h \
    ../../src/process/blackbarprocess.h \
    ../../src/process/scaleprocess.h \
    ../../src/process/lowpassfilterprocess.h \
    ../../src/transform/imagetransform.h \
    ../../src/transform/simpletransform.h \
    ../../src/transform/striptransform.h \
    ../../src/transform/blockstriptransform.h \
    ../../src/transform/blocktransform.h \
    ../../src/leds/leds.h \
    ../../src/common/types.h \
    ../../src/common/util.h
