QT       += core gui widgets
TARGET    = DeepStrata
TEMPLATE  = app
CONFIG   += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mapmanager.cpp \
    buildmanager.cpp \
    survivedata.cpp \
    eventmanager.cpp \
    tileitem.cpp \
    buildingitem.cpp \
    playeritem.cpp \
    monsteritem.cpp \
    texturefactory.cpp

HEADERS += \
    mainwindow.h \
    mapmanager.h \
    buildmanager.h \
    survivedata.h \
    eventmanager.h \
    tileitem.h \
    buildingitem.h \
    playeritem.h \
    monsteritem.h \
    texturefactory.h

RESOURCES += resources.qrc
