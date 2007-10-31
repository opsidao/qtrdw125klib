TEMPLATE = lib

CONFIG += staticlib \
qt \
warn_on
QT += core

SOURCES += qextserialport.cpp \
krw125ctl.cpp
HEADERS += qextserialport.h \
krw125ctl.h
