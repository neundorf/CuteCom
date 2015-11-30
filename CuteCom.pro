#-------------------------------------------------
#
# Project created by QtCreator 2015-11-02T14:38:15
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4){
  QT += widgets
  CONFIG += c++11
}

TARGET = CuteCom
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    controlpanel.cpp \
    settings.cpp \
    devicecombo.cpp \
    serialdevicelistmodel.cpp \
    statusbar.cpp

HEADERS  += mainwindow.h \
    controlpanel.h \
    settings.h \
    devicecombo.h \
    serialdevicelistmodel.h \
    statusbar.h

FORMS    += mainwindow.ui \
    controlpanel.ui \
    statusbar.ui

RESOURCES += \
    resources.qrc
