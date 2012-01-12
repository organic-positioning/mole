# Mole (Mobile Organic Localization Engine)
# Copyright Nokia 2010-2011.
# All rights reserved.

TEMPLATE = app
TARGET = moled

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
UI_DIR = obj

# if using qt mobility 1.2 w/maemo, can do
# export LD_LIBRARY_PATH=/opt/qtm12/lib
# on device
#LIBS += -L/usr/lib/qtm12 -lQtSystemInfo
# Note: forced this to work by moving old library to qtm11
#LIBS += -L/usr/lib/qtm12 -lQtSensors
#LIBS += -L/usr/lib/qtm12 -lQtLocation

QT += core xml network
QT -= gui
#CONFIG += mobility12
CONFIG += mobility
CONFIG += debug
MOBILITY += sensors systeminfo location

HEADERS += \
    src/binder.h \
    src/core.h \
    src/localizer.h \
    src/localServer.h \
    src/moled.h \
    ../common/scanner.h \
    ../common/scan.h \
    src/scanQueue.h \
    src/proximity.h \
    src/speedsensor.h \
    ../common/dbus.h \
    ../common/mole.h \
    ../common/network.h \
    ../common/overlap.h \
    ../common/sig.h \
    ../common/util.h \
    ../common/settings_access.h \
    ../common/version.h

SOURCES += \
    src/binder.cpp \
    src/core.cpp \
    src/localizer.cpp \
    src/localServer.cpp \
    src/localizer_statistics.cpp \
    ../common/scanner.cpp \
#    ../common/scan.cpp \
    src/scanQueue.cpp \
    src/space_parser.cpp \
    src/proximity.cpp \
    src/speedsensor.cpp \
    ../common/mole.cpp \
    ../common/network.cpp \
    ../common/overlap.cpp \
    ../common/sig.cpp \
    ../common/settings_access.cpp \
    ../common/util.cpp

unix:LIBS += -L/usr/lib -lqjson

OTHER_FILES += \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/README

maemo5 {
    CONFIG += icd2 link_pkgconfig
    PKGCONFIG += glib-2.0 icd2
    HEADERS += ../common/scanner-maemo.h  ../common/scan-maemo.h
    SOURCES += ../common/scanner-maemo.cpp
}

!maemo5 {
    HEADERS += ../common/scanner-nm.h
    SOURCES += ../common/scanner-nm.cpp
}

unix:!symbian {
    QT += dbus

    isEmpty(PREFIX) {
      PREFIX = "/usr"
    }
    DATADIR=$$PREFIX/share

    desktopfile.files = data/$${TARGET}.desktop
    maemo5 {
        target.path = $$PREFIX/sbin
        desktopfile.path = $$DATADIR/applications/hildon
    } else {
        target.path = $$PREFIX/local/sbin
        desktopfile.path = $$DATADIR/applications
    }
    service.files = data/$${TARGET}.service
    service.path = $$DATADIR/dbus-1/services

    INSTALLS += target desktopfile service
}

symbian {
    TARGET.UID3 = 0xe689ea0d
    TARGET.CAPABILITY += NetworkServices
    TARGET.CAPABILITY += Location
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}


