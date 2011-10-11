TEMPLATE = app
TARGET = mole-binder

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
UI_DIR = obj

QT += core xml network dbus

#    src/binderui.h \
#    src/statisticswidget.h \
#    src/bindlocationwidget.h \


HEADERS += \
    src/common.h \
    src/binder.h \
    src/places.h \
    src/settings.h \
    ../common/completer.h \
    ../common/models.h \
    ../common/mole.h \
    ../common/network.h \
    ../common/qt-utils.h \
    ../common/util.h \
    ../common/timer.h \
    ../common/version.h

#   src/binderui.cpp \
#    src/statisticswidget.cpp \
#    src/bindlocationwidget.cpp \


SOURCES += \
    src/binder.cpp \
    src/main.cpp \
    src/places.cpp \
    src/settings.cpp \
    ../common/completer.cpp \
    ../common/mole.cpp \
    ../common/models.cpp \
    ../common/network.cpp \
    ../common/qt-utils.cpp \
    ../common/util.cpp \
    ../common/timer.cpp

#FORMS    += \
#    src/binderui.ui \
#    src/statisticswidget.ui \
#    src/bindlocationwidget.ui

# RESOURCES += \
#    mole-binder.qrc

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

OTHER_FILES += \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/README \
    debian/rules
