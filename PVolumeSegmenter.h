/* PVolumeSegmenter.h

   Volume image segmentation tool.

   Copyright 2012, 2103, 2014 National University of Singapore
   Author: Leow Wee Kheng
*/

#ifndef PVOLUMESEGMENTER_H
#define PVOLUMESEGMENTER_H

class QPushButton;
class QHBoxLayout;

#include "PVolumeViewer.h"
#include "PVoiWidget.h"
#include "PThresholder.h"

#include "vtkImageCacheFilter.h"
#include "vtkExtractVOI.h"
#include "vtkMarchingCubes.h"
#include "vtkDecimatePro.h"
#include "vtkSmoothPolyDataFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyDataMapper.h"

#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"

#include "vtkDICOMImageReader.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"


class PVolumeSegmenter: public PVolumeViewer
{
    Q_OBJECT
    
public:
    PVolumeSegmenter();
    ~PVolumeSegmenter();
    
protected:
    void closeEvent(QCloseEvent *event);
    void hideEvent(QHideEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
    
signals:
    void voiChanged();
    
protected slots:
    void saveFile();   // Refine
    void resetZoom();  // Redefine
    void saveMeshPly();
    void saveMeshStl();
    void saveVolumeView();
    void saveMeshView();   
    void info();  // Redefine
    void help();  // Redefine
    
    // Input and processing
    void resetInput();
    void resetProcess();
    
    // VOI
    void resetVoi();
    void setVoi(bool on);
    void cropVoi();
    
    // Tools
    void showThresholder();

    // Volume rendering
    void showVolumeRenderDialog();
    void volumeRender();
    void setSampleDistance(int dist);
    
    // Mesh generation
    void showGenMeshDialog();
    void generateMesh();
    void smoothing();

protected:
    void addWidgets();
    void addActions();
    void addMenus();
    void addToolBars();

    QAction *saveMeshPlyAction;
    QAction *saveMeshStlAction;
    QAction *saveVolumeViewAction;
    QAction *saveMeshViewAction;
    
    QAction *resetInputAction;
    QAction *setVoiAction;
    QAction *cropVoiAction;
    
    QAction *thresholdAction;
    QAction *volumeRenderAction;
    QAction *genMeshAction;
    
    QMenu *saveMeshMenu;
    QMenu *segmenterMenu;
    QMenu *outputMenu;
    
    QToolBar *segmenterToolBar;
    QToolBar *outputToolBar;
    
    // Input/output connection
    // vtkAlgorithmOutput *input;  // Input of process pipleline,
                                   // already defined in PVolumeViewer.
    vtkImageCacheFilter *outputFilter; // Output filter of process pipeline.
    vtkImageData *outputVolume;
    vtkPolyData *outputMesh;
    
    // VOI 1
    PVoiWidget *transVoi;
    PVoiWidget *coronalVoi;
    PVoiWidget *sagittalVoi;
    
    // Selection
    vtkExtractVOI *extractVoi;
        
    // Tools
    PThresholder *thresholder;
    
    // Volume renderer dialog
    QWidget *volumeRenderDialog;
    QComboBox *viewTypeBox;
    QComboBox *sampleDistanceBox;
    QDoubleSpinBox *ambientBox;
    QDoubleSpinBox *specularBox;
    QPushButton *closeVolumeButton;
    
    // Volume renderer: faster visualisation than mesh generation
    QVTKWidget *volumeWidget;
    vtkVolumeMapper *volumeMapper;
    vtkSmartVolumeMapper *smartMapper;
    vtkFixedPointVolumeRayCastMapper *rayCastMapper;
    vtkPiecewiseFunction *opacityFn;
    vtkColorTransferFunction *colorFn;
    vtkVolumeProperty *property;
    vtkVolume *volumeActor;
    vtkRenderer* volumeRenderer;
    vtkInteractorStyleTrackballCamera *volumeStyle;

    // Mesh objects
    vtkMarchingCubes *mcubes;
    vtkDecimatePro *decimate;
    vtkSmoothPolyDataFilter *smooth;
    vtkPolyDataNormals *normals;

    // Mesh generation dialog
    QWidget *genMeshDialog;
    QSpinBox *intensityBox;
    QDoubleSpinBox *ratioBox;
    QDoubleSpinBox *factorBox;
    QSpinBox *smoothIterBox;
    QPushButton *closeMeshButton;
    
    // Mesh viewer
    QHBoxLayout *box01;
    QVTKWidget *meshWidget;
    vtkPolyDataMapper *meshMapper;
    vtkActor *meshActor;
    vtkRenderer *meshRenderer;
    vtkInteractorStyleTrackballCamera *meshStyle;
    
    // Internal variables
    bool voiDone;
    bool thresholdDone;
    bool segmented;
    bool meshDone;
    bool hasVolumeActor;  // Actor added
    bool hasMeshActor;  // Actor added
    int selectedVoi[6];

    // Supporting functions
    void createVoiObjects();
    void createVolumeRenderer();
    void createVolumeDialog();
    void createMeshObjects();
    void createMeshViewer();
    void createMeshDialog();
    void resetPipeline();

    bool saveToFile(const QString &fileName); // Override
    void saveView(int type); // Override
    bool saveView(const QString &fileName, int type); // Override
    double *computeBounds();
    void computeOutputVolume();
    void setBlendType();
};

#endif 