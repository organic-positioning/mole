! include (../common.pri) {
  error(Could not find common.pri)
}

TEMPLATE = app
TARGET = mole-ws

CONFIG += mobility
CONFIG += debug
MOBILITY += systeminfo

HEADERS += \
    ../src/wsClient.h \
    ../src/ports.h \
    ../src/version.h \
    ../src/dbus.h \
    ../src/util.h \
    ../src/source.h \
    ../src/scanner.h \
    ../src/simpleScanQueue.h

SOURCES += \
    ../src/wsClient.cpp \
    ../src/scanner.cpp \
    ../src/util.cpp \
    ../src/source.cpp \
    ../src/simpleScanQueue.cpp

unix:LIBS += -L/usr/lib -lqjson

unix:!symbian {
    QT += dbus

    isEmpty(PREFIX) {
      PREFIX = "/usr"
    }

    maemo5 {
        CONFIG += icd2 link_pkgconfig
        PKGCONFIG += glib-2.0 icd2
        HEADERS += ../src/scanner-maemo.h  ../src/scan-maemo.h
        SOURCES += ../src/scanner-maemo.cpp
        target.path = $$PREFIX/bin
    } else {
        HEADERS += ../src/scanner-nm.h
        SOURCES += ../src/scanner-nm.cpp
        target.path = $$PREFIX/local/bin
    }
    INSTALLS += target
}

#OTHER_FILES += \
#    debian/changelog \
#    debian/compat \
#    debian/control \
#    debian/copyright \
#    debian/README \
#    debian/rules
