TEMPLATE = app
TARGET = mole-binder

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
UI_DIR = obj

QT += core xml network
CONFIG += qdbus mobility debug
MOBILITY += sensors


# Input
HEADERS += src/places.h src/settings.h src/common.h ../common/mole.h ../common/network.h ../common/util.h ../common/qt-utils.h ../common/timer.h ../common/models.h ../common/completer.h src/binder.h
SOURCES += src/main.cpp src/settings.cpp src/places.cpp ../common/network.cpp ../common/util.cpp ../common/mole.cpp ../common/qt-utils.cpp ../common/timer.cpp ../common/models.cpp ../common/completer.cpp src/binder.cpp

LIBS += -L/usr/lib -lqjson
#LIBS += -L../qjson-build-desktop/lib -lqjson


unix:!symbian {
    maemo5 {
        target.path = /opt/usr/bin
    } else {
        target.path = /usr/local/bin
    }
    INSTALLS += target
}


unix:!symbian {
    desktopfile.files = data/$${TARGET}.desktop
    maemo5 {
        desktopfile.path = /usr/share/applications/hildon
    } else {
        desktopfile.path = /usr/share/applications
    }
    INSTALLS += desktopfile
}

unix:!symbian {
    about.files = data/icons/about.png
    about.path = /usr/share/mole/icons

    email.files = data/icons/email.png
    email.path = /usr/share/mole/icons

    feedback.files = data/icons/feedback.png
    feedback.path = /usr/share/mole/icons

    help.files = data/icons/help.png
    help.path = /usr/share/mole/icons

    settings.files = data/icons/settings.png
    settings.path = /usr/share/mole/icons

    INSTALLS += about email feedback help settings
}


unix:!symbian {
  #VARIABLES
  PREFIX = /usr
  BINDIR = $$PREFIX/bin
  DATADIR =$$PREFIX/share

  DEFINES += DATADIR=\\\"$$DATADIR\\\" PKGDATADIR=\\\"$$PKGDATADIR\\\"

  INSTALLS += target iconxpm icon26 icon48 icon64

  target.path =$$BINDIR

  iconxpm.path = $$DATADIR/pixmap
  iconxpm.files += data/maemo/$${TARGET}.xpm

  icon26.path = $$DATADIR/icons/hicolor/26x26/apps
  icon26.files += data/26x26/$${TARGET}.png

  icon48.path = $$DATADIR/icons/hicolor/48x48/apps
  icon48.files += data/48x48/$${TARGET}.png

  icon64.path = $$DATADIR/icons/hicolor/64x64/apps
  icon64.files += data/64x64/$${TARGET}.png
}



OTHER_FILES += \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/README \
    debian/rules \
    mole-binder.desktop

unix:!symbian {
    desktopfile.files = $${TARGET}.desktop
    maemo5 {
        desktopfile.path = /usr/share/applications/hildon
    } else {
        desktopfile.path = /usr/share/applications
    }
    INSTALLS += desktopfile
}
