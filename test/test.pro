TEMPLATE = app

SOURCES += main.cpp \
ventana.cpp
FORMS += ventana.ui

CONFIG += debug \
qt \
warn_on
QT += core \
gui
INCLUDEPATH += ../rdw125k

LIBS += ../rdw125k/librdw125k.a

TARGETDEPS += ../rdw125k/librdw125k.a

HEADERS += ventana.h \
ui_ventana.h
