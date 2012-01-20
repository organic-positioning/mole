! include (../common.pri) {
  error(Could not find common.pri)
}

TEMPLATE = app
TARGET = mole-scanner

CONFIG += mobility
CONFIG += debug
MOBILITY += sensors systeminfo

HEADERS += \
    ../src/scannerDaemon.h \
    ../src/ports.h \
    ../src/util.h \
    ../src/source.h \
    ../src/version.h \
    ../src/scanner.h \
    ../src/speedsensor.h \
    ../src/simpleScanQueue.h

SOURCES += \
    ../src/util.cpp \
    ../src/source.cpp \
    ../src/scannerDaemon.cpp \
    ../src/scanner.cpp \
    ../src/speedsensor.cpp \
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
        target.path = $$PREFIX/sbin
    } else {
        HEADERS += ../src/scanner-nm.h
        SOURCES += ../src/scanner-nm.cpp
        target.path = $$PREFIX/local/sbin
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
