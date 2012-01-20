! include (../common.pri) {
  error(Could not find common.pri)
}

TEMPLATE = app
TARGET = moled

QT += core xml network
QT -= gui
#CONFIG += mobility12
CONFIG += mobility
CONFIG += debug
MOBILITY += sensors systeminfo location

# if using qt mobility 1.2 w/maemo, can do
# export LD_LIBRARY_PATH=/opt/qtm12/lib
# on device
#LIBS += -L/usr/lib/qtm12 -lQtSystemInfo
# Note: forced this to work by moving old library to qtm11
#LIBS += -L/usr/lib/qtm12 -lQtSensors
#LIBS += -L/usr/lib/qtm12 -lQtLocation


HEADERS += \
    ../src/binder.h \
    ../src/daemon.h \
    ../src/localizer.h \
    ../src/localServer.h \
    ../src/scanner.h \
    ../src/scan.h \
    ../src/util.h \
    ../src/source.h \
    ../src/scanQueue.h \
    ../src/proximity.h \
    ../src/speedsensor.h \
    ../src/dbus.h \
    ../src/mole.h \
    ../src/network.h \
    ../src/overlap.h \
    ../src/sig.h \
    ../src/math.h \
    ../src/settings_access.h \
    ../src/version.h

SOURCES += \
    ../src/binder.cpp \
    ../src/daemon.cpp \
    ../src/localizer.cpp \
    ../src/localServer.cpp \
    ../src/localizer_statistics.cpp \
    ../src/scanner.cpp \
#    ../src/scan.cpp \
    ../src/scanQueue.cpp \
    ../src/space_parser.cpp \
    ../src/proximity.cpp \
    ../src/speedsensor.cpp \
    ../src/mole.cpp \
    ../src/network.cpp \
    ../src/overlap.cpp \
    ../src/sig.cpp \
    ../src/settings_access.cpp \
    ../src/math.cpp \
    ../src/util.cpp \
    ../src/source.cpp

unix:LIBS += -L/usr/lib -lqjson

maemo5 {
    CONFIG += icd2 link_pkgconfig
    PKGCONFIG += glib-2.0 icd2
    HEADERS += ../src/scanner-maemo.h  ../src/scan-maemo.h
    SOURCES += ../src/scanner-maemo.cpp
}

unix:!maemo5 {
#    INCLUDEPATH += /usr/include/QtMobility
    HEADERS += ../src/scanner-nm.h
    SOURCES += ../src/scanner-nm.cpp
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

