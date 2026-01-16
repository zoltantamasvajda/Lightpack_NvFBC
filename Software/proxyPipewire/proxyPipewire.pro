QT -= gui
QT += dbus

TEMPLATE = lib

CONFIG += c++17

CONFIG    += link_pkgconfig
PKGCONFIG += libpipewire-0.3

SOURCES += \
    smartPipewire.cpp \
    PipewireHandler.cpp

HEADERS += \
    include/smartPipewire.h \
    include/PipewireHandler.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
DESTDIR     = ../bin
