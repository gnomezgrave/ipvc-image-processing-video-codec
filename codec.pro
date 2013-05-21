TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += main.cpp \
    ipvcplayer.cpp \
    ipvcmain.cpp \
    ipvcencoder.cpp \
    phasecorr.cpp

QMAKE_CXXFLAGS += -O3
win32{
    INCLUDEPATH +=  "C:/opencv-2/build/install/include"
    LIBS += -L"C:/opencv-2/build/install/lib/" -lopencv_core244.dll -lopencv_highgui244.dll
} else {
    LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc # -llzma
}

HEADERS += \
    ipvc.h \
    ipvcplayer.h \
    ipvcmain.h \
    ipvcencoder.h

FORMS += \
    ipvcmain.ui

RESOURCES += \
    res.qrc
