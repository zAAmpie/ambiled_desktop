QT += testlib gui core
QT -= widgets

CONFIG += testcase console c++17
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += \
    ../../src/transform \
    ../../src/leds \
    ../../src/common

SOURCES += \
    tst_transform.cpp \
    ../../src/transform/imagetransform.cpp \
    ../../src/transform/simpletransform.cpp \
    ../../src/transform/striptransform.cpp \
    ../../src/transform/blockstriptransform.cpp \
    ../../src/transform/blocktransform.cpp

HEADERS += \
    ../../src/transform/imagetransform.h \
    ../../src/transform/simpletransform.h \
    ../../src/transform/striptransform.h \
    ../../src/transform/blockstriptransform.h \
    ../../src/transform/blocktransform.h \
    ../../src/leds/leds.h \
    ../../src/common/types.h
