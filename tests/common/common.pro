QT += testlib gui core
QT -= widgets

CONFIG += testcase console c++17
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += \
    ../../src/common

SOURCES += \
    tst_imageutils.cpp \
    ../../src/common/imageutils.cpp

HEADERS += \
    ../../src/common/imageutils.h
