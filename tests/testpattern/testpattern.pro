QT += testlib gui core
QT -= widgets

CONFIG += testcase console c++17
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += \
    ../../src/testpattern \
    ../../src/common

SOURCES += \
    tst_testpattern.cpp \
    ../../src/testpattern/testpattern.cpp

HEADERS += \
    ../../src/testpattern/testpattern.h \
    ../../src/common/parameterspec.h
