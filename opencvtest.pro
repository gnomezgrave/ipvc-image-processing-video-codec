#-------------------------------------------------
#
# Project created by QtCreator 2013-02-20T18:59:55
#
#-------------------------------------------------

QT       += core gui

TARGET = opencvtest
TEMPLATE = app

win32{
    INCLUDEPATH +=  "C:/opencv-2/build/install/include"
    LIBS += -L"C:/opencv-2/build/install/lib/" -lopencv_core244.dll -lopencv_highgui244.dll
} else {
    LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc
}
SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
