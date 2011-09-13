TEMPLATE = app
TARGET = mole

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
UI_DIR = obj

QT += core network
QT -= gui

HEADERS += \
    ../common/ports.h \
    ../common/version.h

SOURCES += \
    src/main.cpp

unix:LIBS += -L/usr/lib -lqjson

unix:!symbian {
    isEmpty(PREFIX) {
      PREFIX = "/usr"
    }

    maemo5 {
        target.path = $$PREFIX/bin
    } else {
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
