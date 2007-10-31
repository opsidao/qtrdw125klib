TEMPLATE = app

SOURCES += main.cpp \
ventana.cpp
FORMS += ventana.ui

CONFIG += debug \
qt \
warn_on
QT += core \
gui
INCLUDEPATH += ../library

LIBS += ../library/liblibrary.a

TARGETDEPS += ../library/liblibrary.a

HEADERS += ventana.h \
ui_ventana.h
