TEMPLATE = subdirs

SUBDIRS = \
    transform \
    process \
    common \
    testpattern \
    bench

#The capture tests exercise the X11 backend, which only exists on Unix
unix: SUBDIRS += capture
