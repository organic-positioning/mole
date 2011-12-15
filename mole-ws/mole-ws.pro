TEMPLATE = app
TARGET = moleWS

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
UI_DIR = obj

QT += core network
QT -= gui
CONFIG += debug

HEADERS += \
    src/core.h \
    ../common/ports.h \
    ../common/version.h \
    ../common/dbus.h \
    ../common/scanner.h \
    src/simpleScanQueue.h

SOURCES += \
    src/core.cpp \
    ../common/scanner.cpp \
    src/simpleScanQueue.cpp

unix:LIBS += -L/usr/lib -lqjson

unix:!symbian {
    QT += dbus

    isEmpty(PREFIX) {
      PREFIX = "/usr"
    }

    maemo5 {
        CONFIG += icd2 link_pkgconfig
        PKGCONFIG += glib-2.0 icd2
        HEADERS += ../common/scanner-maemo.h  ../common/scan-maemo.h
        SOURCES += ../common/scanner-maemo.cpp
        target.path = $$PREFIX/bin
    } else {
        HEADERS += ../common/scanner-nm.h
        SOURCES += ../common/scanner-nm.cpp
        target.path = $$PREFIX/local/bin
    }
    INSTALLS += target
}

OTHER_FILES += \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/README \
    debian/rules
