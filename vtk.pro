QMAKE_CXXFLAGS += -Wno-deprecated 
INCLUDEPATH += /usr/local/include/vtk /usr/include/vtk-5.8
LIBS += -L/usr/local/lib/vtk -lQVTK -lvtkalglib -lvtkCharts -lvtkCommon -lvtkDICOMParser -lvtkFiltering -lvtkftgl -lvtkGenericFiltering -lvtkGraphics -lvtkHybrid -lvtkImaging -lvtkIO -lvtkRendering -lvtksys -lvtkViews -lvtkVolumeRendering -lvtkWidgets -ldl -lvtkHybrid
# below libs are currently not used and are not installed on my MacOS -- LH
