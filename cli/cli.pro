! include (../common.pri) {
  error(Could not find common.pri)
}

TEMPLATE = app
TARGET = mole

HEADERS += \
    ../src/ports.h \
    ../src/version.h

SOURCES += \
    ../src/cli.cpp

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
