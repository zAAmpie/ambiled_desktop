TEMPLATE = subdirs

SUBDIRS = \
    transform \
    process \
    common \
    bench

#The capture tests exercise the X11 backend, which only exists on Unix
unix: SUBDIRS += capture
