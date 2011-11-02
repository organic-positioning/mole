# Mole (Mobile Organic Localization Engine)
# Copyright Nokia 2010-2011.
# All rights reserved.

TEMPLATE = app
TARGET = moled

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
UI_DIR = obj

QT += core xml network
QT -= gui
CONFIG += mobility
CONFIG += debug
MOBILITY += sensors systeminfo

HEADERS += \
    src/binder.h \
    src/core.h \
    src/localizer.h \
    src/localServer.h \
    src/moled.h \
    src/scan.h \
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
    src/scan.cpp \
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
    HEADERS += src/scanner-maemo.h  src/scan-maemo.h
    SOURCES += src/scanner-maemo.cpp
}

!maemo5 {
    HEADERS += src/scanner-nm.h
    SOURCES += src/scanner-nm.cpp
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
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}
