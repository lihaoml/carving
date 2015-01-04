TEMPLATE = app
TARGET = carving
DESTDIR = ./_make/
MOC_DIR = ./_make/
QRC_DIR = ./_make/
RCC_DIR = ./_make/
OBJECTS_DIR = ./_make/
DEPENDPATH += .
INCLUDEPATH += .
RESOURCES += panax.qrc
include(./vtk.pro)

QMAKE_CXXFLAGS += -Wno-unused-variable -fpermissive -Wno-unused-parameter

LIBS += -L/usr/local/lib 

# Input
HEADERS += PBrainExtractor.h \
           PThresholder.h \
           PVoiWidget.h \
           PVolumeSegmenter.h \
           PVolumeViewer.h
SOURCES += main.cpp \
           PBrainExtractor.cpp \
           PThresholder.cpp \
           PVoiWidget.cpp \
           PVolumeSegmenter.cpp \
           PVolumeViewer.cpp
