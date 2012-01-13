! include (../common.pri) {
  error(Could not find common.pri)
}

TEMPLATE = app
TARGET = mole-binder

QT += gui
QT += xml network dbus

HEADERS += \
    ../src/gui/common.h \
    ../src/gui/binderGui.h \
    ../src/gui/places.h \
    ../src/gui/settings.h \
    ../src/gui/updatingDisplay.h \
    ../src/completer.h \
    ../src/models.h \
    ../src/mole.h \
    ../src/network.h \
    ../src/qt-utils.h \
    ../src/math.h \
    ../src/settings_access.h \
    ../src/timer.h \
    ../src/version.h

SOURCES += \
    ../src/gui/binderGui.cpp \
    ../src/gui/gui.cpp \
    ../src/gui/places.cpp \
    ../src/gui/settings.cpp \
    ../src/gui/updatingDisplay.cpp \
    ../src/completer.cpp \
    ../src/mole.cpp \
    ../src/models.cpp \
    ../src/network.cpp \
    ../src/qt-utils.cpp \
    ../src/math.cpp \
    ../src/settings_access.cpp \
    ../src/timer.cpp


unix:LIBS += -L/usr/lib -lqjson

unix:!symbian {
    isEmpty(PREFIX) {
      PREFIX = "/usr"
    }

    DATADIR =$$PREFIX/share

    DEFINES += DATADIR=\"$$DATADIR\" PKGDATADIR=\"$$PKGDATADIR\"

    INSTALLS += target desktopfile about email feedback help settings icon icon26 icon48 icon64

    desktopfile.files = data/$${TARGET}.desktop

    maemo5 {
        target.path = $$PREFIX/bin
        desktopfile.path = $$DATADIR/applications/hildon
    } else {
        target.path = $$PREFIX/local/bin
        desktopfile.path = $$DATADIR/applications
    }

    about.files = data/icons/about.png
    about.path = $$DATADIR/mole/icons

    email.files = data/icons/email.png
    email.path = $$DATADIR/mole/icons

    feedback.files = data/icons/feedback.png
    feedback.path = $$DATADIR/mole/icons

    help.files = data/icons/help.png
    help.path = $$DATADIR/mole/icons

    settings.files = data/icons/settings.png
    settings.path = $$DATADIR/mole/icons

    icon.files += data/icons/$${TARGET}.png
    icon.path = $$DATADIR/mole/icons

    icon26.files += data/icons/26x26/$${TARGET}.png
    icon26.path = $$DATADIR/icons/hicolor/26x26/apps

    icon48.files += data/icons/48x48/$${TARGET}.png
    icon48.path = $$DATADIR/icons/hicolor/48x48/apps

    icon64.files += data/icons/64x64/$${TARGET}.png
    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
}
