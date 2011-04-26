# Mole (Mobile Organic Localization Engine)
# Copyright Nokia 2010.
# All rights reserved.

TEMPLATE = app
TARGET = moled

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
UI_DIR = obj

QT += core xml network
CONFIG += qdbus mobility debug
MOBILITY += sensors systeminfo

maemo5 {
  CONFIG += icd2 link_pkgconfig
  PKGCONFIG += glib-2.0 icd2
  HEADERS += src/scanner-maemo.h  src/scan-maemo.h
  SOURCES += src/scanner-maemo.cpp
}

!maemo5 {
  HEADERS += src/scanner.h
  SOURCES += src/scanner.cpp
}


# MOBILITY += systeminfo bearer

HEADERS += src/binder.h ../common/util.h src/scan.h src/localizer.h src/core.h src/speedsensor.h ../common/overlap.h ../common/network.h ../common/sig.h src/moled.h ../common/mole.h 
SOURCES += src/binder.cpp ../common/util.cpp src/scan.cpp src/localizer.cpp src/core.cpp src/speedsensor.cpp ../common/overlap.cpp ../common/network.cpp ../common/sig.cpp ../common/mole.cpp 


FORMS += 
RESOURCES +=

symbian {
    TARGET.UID3 = 0xe689ea0d
    # TARGET.CAPABILITY +=
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}



LIBS += -L/usr/lib -lqjson


OTHER_FILES += \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/README


unix:!symbian {
    maemo5 {
        target.path = /opt/usr/sbin
    } else {
        target.path = /usr/local/sbin
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
