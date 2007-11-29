TEMPLATE = lib

CONFIG += staticlib \
	qt \
	warn_on
QT += core

SOURCES += qextserialport.cpp \
	qt_rdw125k.cpp
HEADERS += qextserialport.h \
	qt_rdw125k.h
