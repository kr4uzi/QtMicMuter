#-------------------------------------------------
#
# Project created by QtCreator 2015-03-17T22:05:53
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtMicMuter
TEMPLATE = app

SOURCES += main.cpp KeyboardHook.cpp MicMuter.cpp Microphone.cpp MicMuterGUI.cpp
HEADERS += KeyboardHook.h MicMuter.h Microphone.h MicMuterGUI.h
RESOURCES += iconresource.qrc
CONFIG += no_keywords static

win32 {
    HEADERS += ComInterface.h
    SOURCES += ComInterface.cpp
    LIBS += -lOle32 -lUser32
    INCLUDEPATH += C:/boost_1_57_0/
    DEFINES += "_SCL_SECURE_NO_WARNINGS"
}

macx {
    HEADERS += AudioDeviceInterface.h
    SOURCES += AudioDeviceInterface.cpp
    LIBS += -framework CoreAudio ApplicationServices
}
