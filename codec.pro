TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -O3
win32{
    INCLUDEPATH +=  "C:/opencv-2/build/install/include"
    LIBS += -L"C:/opencv-2/build/install/lib/" -lopencv_core244.dll -lopencv_highgui244.dll
} else {
    LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc # -llzma
}
