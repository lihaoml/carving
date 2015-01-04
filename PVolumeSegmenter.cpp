/* PVolumeSegmenter.cpp

   Volume image segmentation tool.

   Copyright 2012, 2103, 2014 National University of Singapore
   Author: Leow Wee Kheng
*/

#include "PVolumeSegmenter.h"
#include <QtGui>

#include "vtkPLYWriter.h"
#include "vtkSTLWriter.h"
#include "vtkCellArray.h"
#include "vtkProperty.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleImage.h"
#include "vtkWindowToImageFilter.h"

#include "vtkDICOMImageReader.h"
#include "vtkMetaImageReader.h"
#include "vtkMetaImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkTIFFWriter.h"

#include <unistd.h>
using namespace std;

#define USE_SMART_MAPPER
// #define DEBUG


PVolumeSegmenter::PVolumeSegmenter()
{
    outputVolume = NULL;
    outputMesh = NULL;
    
    loaded = false;
    voiDone = false;
    thresholdDone = false;
    segmented = false;
    meshDone = false;
    hasVolumeActor = false;
    hasMeshActor = false;
    
    addWidgets();
    addActions();
    addMenus();
    addToolBars();

    appName = QString("Volume Segmenter");
    setWindowTitle(appName);
    
    winWidth = 830;
    winHeight = 710;
    setMinimumSize(winWidth, winHeight);
    transWidget->updateGeometry();
    coronalWidget->updateGeometry();
    sagittalWidget->updateGeometry();
}


PVolumeSegmenter::~PVolumeSegmenter()
{
    delete transVoi;
    delete coronalVoi;
    delete sagittalVoi;
    delete thresholder;
    delete volumeRenderDialog;
    delete genMeshDialog;
  
    extractVoi->Delete();     
    
    colorFn->Delete();
    opacityFn->Delete();
    property->Delete();
#ifdef USE_SMART_MAPPER
    smartMapper->Delete();
#endif
    rayCastMapper->Delete();
    volumeActor->Delete();
    volumeRenderer->RemoveAllViewProps();
    volumeWidget->GetRenderWindow()->RemoveRenderer(volumeRenderer);
    volumeRenderer->Delete();
    volumeStyle->Delete();
    
    mcubes->Delete();
    decimate->Delete();
    smooth->Delete();
    normals->Delete();
    meshMapper->Delete();
    meshActor->Delete();
    meshRenderer->RemoveAllViewProps();
    meshWidget->GetRenderWindow()->RemoveRenderer(meshRenderer);
    meshRenderer->Delete();
    meshStyle->Delete();

    outputFilter->Delete();
    outputVolume->Delete();
    outputMesh->Delete();
}


// Qt event handlers

void PVolumeSegmenter::closeEvent(QCloseEvent *event)
{
    thresholder->hide();
    volumeRenderDialog->hide();
    genMeshDialog->hide();
    event->accept();
}


void PVolumeSegmenter::hideEvent(QHideEvent *event)
{
    thresholder->hide();
    volumeRenderDialog->hide();
    genMeshDialog->hide();
    event->accept();
}


bool PVolumeSegmenter::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Close)
    {
        if (object == volumeRenderDialog)
        {
            volumeRenderAction->setChecked(false);
            return false;
        }
        else if (object == genMeshDialog)
        {
            genMeshAction->setChecked(false);
            return false;
        }
    }
        
    return PVolumeViewer::eventFilter(object, event);
}


// Create additional widgets and objects

void PVolumeSegmenter::addWidgets()
{
    // Create output filter.
    outputFilter = vtkImageCacheFilter::New();
    
    // Create VOI objects and connect signal.
    createVoiObjects();
    connect(this, SIGNAL(voiChanged()), this, SLOT(resetProcess()));

    // Create thresholder and connect signal.
    thresholder = new PThresholder;
    connect(thresholder, SIGNAL(updated()), this, SLOT(updateViewers()));
    
    // Create volumer renderer and mesh generator.
    createVolumeRenderer();
    createVolumeDialog();
    createMeshObjects();
    createMeshViewer();
    createMeshDialog();
    
    // Set pane01
    box01 = new QHBoxLayout;
    box01->addWidget(volumeWidget);
    box01->addWidget(meshWidget);
    meshWidget->hide();
    pane01->setLayout(box01);
    
    // Install event filters
    volumeRenderDialog->installEventFilter(this);
    genMeshDialog->installEventFilter(this);
}


void PVolumeSegmenter::createVoiObjects()
{
    // VOI #1
    transVoi = new PVoiWidget;
    transVoi->alignXYPlane();
    transVoi->setColor(PVoiWidget::Top, 255, 255, 0);
    transVoi->setColor(PVoiWidget::Bottom, 0, 255, 255);
    transVoi->setColor(PVoiWidget::Left, 255, 255, 0);
    transVoi->setColor(PVoiWidget::Right, 0, 255, 255);
    
    coronalVoi = new PVoiWidget;
    coronalVoi->alignXZPlane();
    coronalVoi->setColor(PVoiWidget::Top, 255, 255, 0);
    coronalVoi->setColor(PVoiWidget::Bottom, 0, 255, 255);
    coronalVoi->setColor(PVoiWidget::Left, 255, 255, 0);
    coronalVoi->setColor(PVoiWidget::Right, 0, 255, 255);
    
    sagittalVoi = new PVoiWidget;
    sagittalVoi->alignYZPlane();
    sagittalVoi->setColor(PVoiWidget::Top, 255, 255, 0);
    sagittalVoi->setColor(PVoiWidget::Bottom, 0, 255, 255);
    sagittalVoi->setColor(PVoiWidget::Left, 255, 255, 0);
    sagittalVoi->setColor(PVoiWidget::Right, 0, 255, 255);
            
    connect(transVoi, SIGNAL(topLineChanged(double)),
        sagittalVoi, SLOT(setLeftLine(double)));
    connect(transVoi, SIGNAL(bottomLineChanged(double)),
        sagittalVoi, SLOT(setRightLine(double)));
    connect(transVoi, SIGNAL(leftLineChanged(double)),
        coronalVoi, SLOT(setLeftLine(double)));
    connect(transVoi, SIGNAL(rightLineChanged(double)),
        coronalVoi, SLOT(setRightLine(double)));
    
    connect(coronalVoi, SIGNAL(topLineChanged(double)),
        sagittalVoi, SLOT(setTopLine(double)));
    connect(coronalVoi, SIGNAL(bottomLineChanged(double)),
        sagittalVoi, SLOT(setBottomLine(double)));
    connect(coronalVoi, SIGNAL(leftLineChanged(double)),
        transVoi, SLOT(setLeftLine(double)));
    connect(coronalVoi, SIGNAL(rightLineChanged(double)),
        transVoi, SLOT(setRightLine(double)));

    connect(sagittalVoi, SIGNAL(topLineChanged(double)),
        coronalVoi, SLOT(setTopLine(double)));
    connect(sagittalVoi, SIGNAL(bottomLineChanged(double)),
        coronalVoi, SLOT(setBottomLine(double)));
    connect(sagittalVoi, SIGNAL(leftLineChanged(double)),
        transVoi, SLOT(setTopLine(double)));
    connect(sagittalVoi, SIGNAL(rightLineChanged(double)),
        transVoi, SLOT(setBottomLine(double)));
        
    connect(transVoi, SIGNAL(topLineChanged(double)),
        this, SLOT(updateViewers()));
    connect(coronalVoi, SIGNAL(leftLineChanged(double)),
        this, SLOT(updateViewers()));
    connect(sagittalVoi, SIGNAL(rightLineChanged(double)),
        this, SLOT(updateViewers()));
        
    extractVoi = vtkExtractVOI::New();
    extractVoi->SetSampleRate(1, 1, 1);
    // extractVoi's input is not yet set.
}


void PVolumeSegmenter::createVolumeRenderer()
{
    // Create volume renderer widget
    volumeWidget = new QVTKWidget;
    outputVolume = vtkImageData::New();
        
    // Create transfer functions
    opacityFn = vtkPiecewiseFunction::New();
    colorFn = vtkColorTransferFunction::New();
  
    // Create volume property and attach transfer functions
    property = vtkVolumeProperty::New();
    property->SetIndependentComponents(true);
    property->SetScalarOpacity(opacityFn);
    property->SetColor(colorFn);
    property->SetInterpolationTypeToLinear();
    
    // Create mapper and actor
    rayCastMapper = vtkFixedPointVolumeRayCastMapper::New();
    volumeMapper = rayCastMapper;
    
#ifdef USE_SMART_MAPPER
    smartMapper = vtkSmartVolumeMapper::New();
    volumeMapper = smartMapper;  // Default
#endif

    // mapper's input is not yet connected
    volumeActor = vtkVolume::New();
    volumeActor->SetProperty(property);
    // actor's mapper is not yet set
    
    // Create renderer
    volumeRenderer = vtkRenderer::New();
    volumeRenderer->SetBackground(0.0, 0.0, 0.0);
    vtkRenderWindow *renderWindow = volumeWidget->GetRenderWindow();
    renderWindow->AddRenderer(volumeRenderer);
    // actor is not yet added to renderer
    
    // Set interactor style
    vtkRenderWindowInteractor *interactor = renderWindow->GetInteractor();
    volumeStyle = vtkInteractorStyleTrackballCamera::New();
    interactor->SetInteractorStyle(volumeStyle);
}


void PVolumeSegmenter::createVolumeDialog()
{
    // Volume render dialog
    volumeRenderDialog = new QWidget;
    volumeRenderDialog->setWindowTitle("Volume Renderer");
    volumeRenderDialog->setFixedSize(250, 250);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    volumeRenderDialog->setLayout(mainLayout);
    
    // Viewing mode setting
    QGroupBox *gbox = new QGroupBox("Viewing Mode");
    viewTypeBox = new QComboBox;
    QStringList choices;
    choices << "CT Skin" << "CT Muscle" << "CT Bone" << "CTA Vessels";
    viewTypeBox->insertItems(0, choices);
    
    sampleDistanceBox = new QComboBox;
    choices.clear();
    choices << "1" << "1/2" << "1/4" << "1/8" << "1/16";
    sampleDistanceBox->insertItems(0, choices);
    connect(sampleDistanceBox, SIGNAL(activated(int)),
        this, SLOT(setSampleDistance(int)));
        
    ambientBox = new QDoubleSpinBox;
    ambientBox->setDecimals(2);
    ambientBox->setRange(0.0, 1.0);
    ambientBox->setSingleStep(0.1);
    ambientBox->setValue(0.1);
    
    specularBox = new QDoubleSpinBox;
    specularBox->setDecimals(2);
    specularBox->setRange(0.0, 1.0);
    specularBox->setSingleStep(0.1);
    specularBox->setValue(0.2);
        
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("mode"), 0, 0);
    grid->addWidget(viewTypeBox, 0, 1);
    grid->addWidget(new QLabel("sampling"), 1, 0);
    grid->addWidget(sampleDistanceBox, 1, 1);
    grid->addWidget(new QLabel("ambient"), 2, 0);
    grid->addWidget(ambientBox, 2, 1);
    grid->addWidget(new QLabel("specular"), 3, 0);
    grid->addWidget(specularBox, 3, 1);
    gbox->setLayout(grid);
    mainLayout->addWidget(gbox);
    
    // Apply and close buttons
    QPushButton *abutton = new QPushButton("apply");
    connect(abutton, SIGNAL(clicked()), this, SLOT(volumeRender()));
    closeVolumeButton = new QPushButton("close");
    connect(closeVolumeButton, SIGNAL(clicked()),
        volumeRenderDialog, SLOT(hide()));
    
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(abutton);
    hbox->addStretch();
    hbox->addWidget(closeVolumeButton);
    mainLayout->addLayout(hbox);
    
    setBlendType();
}


void PVolumeSegmenter::createMeshObjects()
{
    // Marching cubes and post-processors
    mcubes = vtkMarchingCubes::New();
    mcubes->ComputeScalarsOff();   // Speed up processing.
    mcubes->ComputeNormalsOff();   // Speed up processing.
    mcubes->ComputeGradientsOff(); // Speed up processing.
    // input to mcubes is not yet set.

    double featureAngle = 60.0;
    decimate = vtkDecimatePro::New();
    decimate->SetFeatureAngle(featureAngle);
    decimate->PreserveTopologyOn();
    // mcubes to decimate is not yet connected.
    
    smooth = vtkSmoothPolyDataFilter::New();
    smooth->SetFeatureAngle(featureAngle);
    smooth->FeatureEdgeSmoothingOff();
    smooth->BoundarySmoothingOff();
    smooth->SetConvergence(0);
    smooth->SetInputConnection(decimate->GetOutputPort());

    normals = vtkPolyDataNormals::New();
    normals->SetFeatureAngle(featureAngle);
    // mcubes or smooth to normals is not yet connected.
}


void PVolumeSegmenter::createMeshViewer()
{
    // Create mesh viewer widget
    meshWidget = new QVTKWidget;
    outputMesh = vtkPolyData::New();

    // Create mapper and actor
    meshMapper = vtkPolyDataMapper::New();
    // mapper's input is not yet connected
    meshActor = vtkActor::New();
    // actor's mapper is not yet set
    
    // Create renderer
    meshRenderer = vtkRenderer::New();
    meshRenderer->SetBackground(0.0, 0.0, 0.0);
    vtkRenderWindow *renderWindow = meshWidget->GetRenderWindow();
    renderWindow->AddRenderer(meshRenderer);
    // actor is not yet added to renderer
    
    // Set interaction style
    vtkRenderWindowInteractor *interactor = renderWindow->GetInteractor();
    meshStyle = vtkInteractorStyleTrackballCamera::New();
    interactor->SetInteractorStyle(meshStyle);
}


void PVolumeSegmenter::createMeshDialog()
{
    // Mesh generation dialog
    genMeshDialog = new QWidget;
    genMeshDialog->setWindowTitle("Mesh Generation");
    genMeshDialog->setFixedSize(300, 350);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    genMeshDialog->setLayout(mainLayout);
    
    // Intensity setting
    QGroupBox *gbox = new QGroupBox("Surface Intensity");
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(-200, 500);
    intensityBox = new QSpinBox;
    intensityBox->setRange(-200, 500);
    intensityBox->setValue(0);
    connect(slider, SIGNAL(valueChanged(int)),
        intensityBox, SLOT(setValue(int)));
    connect(intensityBox, SIGNAL(valueChanged(int)),
        slider, SLOT(setValue(int)));

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel("value"));
    hbox->addWidget(intensityBox);
    hbox->addWidget(slider);
    gbox->setLayout(hbox);
    
    // Apply button
    QPushButton *abutton = new QPushButton("apply");
    connect(abutton, SIGNAL(clicked()), this, SLOT(generateMesh()));
    hbox = new QHBoxLayout;
    hbox->addWidget(abutton);
    hbox->addStretch();
    
    mainLayout->addWidget(gbox);
    mainLayout->addLayout(hbox);
    mainLayout->addWidget(
        new QLabel("_____________________________________"));
    mainLayout->addSpacing(7);
    
    // Reduce resolution
    gbox = new QGroupBox("Reduce Resolution");
    ratioBox = new QDoubleSpinBox;
    ratioBox->setDecimals(2);
    ratioBox->setRange(0.0, 1.0);
    ratioBox->setSingleStep(0.1);
    ratioBox->setValue(0.3);
    
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("ratio        "), 0, 0);
    grid->addWidget(ratioBox, 0, 1);
    
    hbox = new QHBoxLayout;
    hbox->addLayout(grid);
    hbox->addStretch();
    gbox->setLayout(hbox);
    mainLayout->addWidget(gbox);
    
    // Surface Smoothing
    gbox = new QGroupBox("Surface Smoothing");
    factorBox = new QDoubleSpinBox;
    factorBox->setDecimals(2);
    factorBox->setRange(0.0, 1.0);
    factorBox->setSingleStep(0.1);
    factorBox->setValue(0.1);
    
    smoothIterBox = new QSpinBox;
    smoothIterBox->setRange(1, 30);
    smoothIterBox->setValue(1);
    
    grid = new QGridLayout;
    grid->addWidget(new QLabel("factor"), 0, 0);
    grid->addWidget(factorBox, 0, 1);
    grid->addWidget(new QLabel("iteration"), 1, 0);
    grid->addWidget(smoothIterBox, 1, 1);
    
    hbox = new QHBoxLayout;
    hbox->addLayout(grid);
    hbox->addStretch();
    gbox->setLayout(hbox);
    mainLayout->addWidget(gbox);
    
    // Apply and close buttons
    abutton = new QPushButton("apply");
    connect(abutton, SIGNAL(clicked()), this, SLOT(smoothing()));
    closeMeshButton = new QPushButton("close");
    connect(closeMeshButton, SIGNAL(clicked()),
        genMeshDialog, SLOT(hide()));
    
    hbox = new QHBoxLayout;
    hbox->addWidget(abutton);
    hbox->addStretch();
    hbox->addWidget(closeMeshButton);
    mainLayout->addLayout(hbox);
}


void PVolumeSegmenter::addActions()
{
    connect(loadDirAction, SIGNAL(triggered()), this, SLOT(resetInput()));
    connect(loadDirAction, SIGNAL(triggered()), this, SLOT(resetVoi()));
    
    connect(loadFileAction, SIGNAL(triggered()), this, SLOT(resetInput()));
    connect(loadFileAction, SIGNAL(triggered()), this, SLOT(resetVoi()));
    
    disconnect(saveFileAction, 0, 0, 0);  // Refine
    connect(saveFileAction, SIGNAL(triggered()), this, SLOT(saveFile()));

    disconnect(resetZoomAction, 0, 0, 0);  // Redefine
    connect(resetZoomAction, SIGNAL(triggered()), this, SLOT(resetZoom()));
    
    disconnect(infoAction, 0, 0, 0);  // Redefine
    connect(infoAction, SIGNAL(triggered()), this, SLOT(info()));
    
    disconnect(helpAction, 0, 0, 0);  // Redefine
    connect(helpAction, SIGNAL(triggered()), this, SLOT(help()));
    
    saveMeshPlyAction = new QAction(tr("in PLY format"), this);
    connect(saveMeshPlyAction, SIGNAL(triggered()),
        this, SLOT(saveMeshPly()));

    saveMeshStlAction = new QAction(tr("in STL format"), this);
    connect(saveMeshStlAction, SIGNAL(triggered()),
        this, SLOT(saveMeshStl()));
        
    saveVolumeViewAction = new QAction(tr("&Volume view"), this);
    saveVolumeViewAction->setStatusTip(tr("Save volume view to a file"));
    connect(saveVolumeViewAction, SIGNAL(triggered()),
        this, SLOT(saveVolumeView()));
        
    saveMeshViewAction = new QAction(tr("&Mesh view"), this);
    saveMeshViewAction->setStatusTip(tr("Save mesh view to a file"));
    connect(saveMeshViewAction, SIGNAL(triggered()),
        this, SLOT(saveMeshView()));
    
    resetInputAction = new QAction(tr("Reset Input"), this);
    resetInputAction->setIcon(QIcon(":/images/reset.png"));
    resetInputAction->setStatusTip(tr("Reset to original input."));
    connect(resetInputAction, SIGNAL(triggered()),
        this, SLOT(resetInput()));

    setVoiAction = new QAction(tr("Set &VOI"), this);
    setVoiAction->setIcon(QIcon(":/images/roi.png"));
    setVoiAction->setStatusTip(tr("Set volume of interest."));
    setVoiAction->setCheckable(true);
    connect(setVoiAction, SIGNAL(toggled(bool)),
        this, SLOT(setVoi(bool)));
    
    cropVoiAction = new QAction(tr("Crop VOI"), this);
    cropVoiAction->setIcon(QIcon(":/images/cut.png"));
    cropVoiAction->setStatusTip(tr("Crop volume of interest."));
    connect(cropVoiAction, SIGNAL(triggered()), this, SLOT(cropVoi()));
        
    thresholdAction = new QAction(tr("&Thresholding"), this);
    thresholdAction->setIcon(QIcon(":/images/halfwhite.png"));
    thresholdAction->setShortcut(tr("Ctrl+T"));
    thresholdAction->setStatusTip(tr("Apply thresholding to image."));
    thresholdAction->setCheckable(true);
    connect(thresholdAction, SIGNAL(triggered()),
        this, SLOT(showThresholder()));
    connect(thresholder, SIGNAL(closed()),
        thresholdAction, SLOT(toggle()));
           
    volumeRenderAction = new QAction(tr("Volume &Render"), this);
    volumeRenderAction->setIcon(QIcon(":/images/volume.png"));
    volumeRenderAction->setStatusTip(tr("Volume render segmented volumes."));
    volumeRenderAction->setCheckable(true);
    connect(volumeRenderAction, SIGNAL(triggered()),
        this, SLOT(showVolumeRenderDialog()));
    connect(closeVolumeButton, SIGNAL(clicked()),
        volumeRenderAction, SLOT(toggle()));
        
    genMeshAction = new QAction(tr("Generate &Mesh"), this);
    genMeshAction->setIcon(QIcon(":/images/mesh.png"));
    genMeshAction->setShortcut(tr("Ctrl+G"));
    genMeshAction->setStatusTip(tr("Generate mesh model."));
    genMeshAction->setCheckable(true);
    connect(genMeshAction, SIGNAL(triggered()),
        this, SLOT(showGenMeshDialog()));
    connect(closeMeshButton, SIGNAL(clicked()),
        genMeshAction, SLOT(toggle()));
                
    connect(volumeRenderAction, SIGNAL(toggled(bool)),
        genMeshAction, SLOT(setDisabled(bool)));
    connect(genMeshAction, SIGNAL(toggled(bool)),
        volumeRenderAction, SLOT(setDisabled(bool)));
}


void PVolumeSegmenter::addMenus()
{
    saveMeshMenu = new QMenu(tr("Save &Meshes"));
    saveMeshMenu->setIcon(QIcon("images/save.png"));
    fileMenu->insertMenu(exitAction, saveMeshMenu);
    saveMeshMenu->addAction(saveMeshPlyAction);
    saveMeshMenu->addAction(saveMeshStlAction);
    
    saveViewMenu->addAction(saveVolumeViewAction);
    saveViewMenu->addAction(saveMeshViewAction);
    
    segmenterMenu = new QMenu(tr("Segmenter"));
    menuBar()->insertMenu(helpMenu->menuAction(), segmenterMenu);
    segmenterMenu->addAction(resetInputAction);
    segmenterMenu->addAction(setVoiAction);
    segmenterMenu->addAction(cropVoiAction);
    segmenterMenu->addAction(thresholdAction);
    
    outputMenu = new QMenu(tr("Output"));
    menuBar()->insertMenu(helpMenu->menuAction(), outputMenu);
    outputMenu->addAction(volumeRenderAction);
    outputMenu->addAction(genMeshAction);
}


void PVolumeSegmenter::addToolBars()
{   
    segmenterToolBar = new QToolBar(tr("Segmenter"));
    insertToolBar(helpToolBar, segmenterToolBar);
    segmenterToolBar->addAction(resetInputAction);
    segmenterToolBar->addAction(setVoiAction);
    segmenterToolBar->addAction(cropVoiAction);
    segmenterToolBar->addAction(thresholdAction);
    
    outputToolBar = new QToolBar(tr("Output"));
    insertToolBar(helpToolBar, outputToolBar);
    outputToolBar->addAction(volumeRenderAction);
    outputToolBar->addAction(genMeshAction);
}


// Slot functions

void PVolumeSegmenter::saveFile()
{
    if (!loaded)
    {
        QMessageBox::critical(this, appName,
            QString("No volume image to save."));
        return;
    }
    
    QString	fileName = QFileDialog::getSaveFileName(this, 
        tr("Save volume image into a file"), writeDir,
        tr("Volume image: *.mha, *.mhd (*.mha *.mhd)"));
        
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix != "mha" && suffix != "mhd")
    {
        QMessageBox::critical(this, appName,
            QString("File type %1 is unsupported.").arg(suffix));
        return;
    }
        
    if (!fileName.isEmpty())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        if (saveToFile(fileName))
            writeDir = QFileInfo(fileName).path();
        QApplication::restoreOverrideCursor();
    }
}


void PVolumeSegmenter::resetZoom()
{
    setVoiAction->setChecked(false);
    PVolumeViewer::resetZoom();
}


void PVolumeSegmenter::saveMeshPly()
{
    if (!outputMesh)
    {
        QMessageBox::critical(this, appName,
            "No segmented mesh model to save.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save segmented mesh model into a PLY file"), ".",
        tr("PLY file: *.ply (*.ply)"));
        
    vtkPLYWriter *writer = vtkPLYWriter::New();
    writer->SetFileName(fileName.toAscii().data());
    writer->SetInput(outputMesh);
    writer->Update();
    writer->Delete();
}


void PVolumeSegmenter::saveMeshStl()
{
    if (!outputMesh)
    {
        QMessageBox::critical(this, appName,
            "No segmented mesh model to save.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save segmented mesh model into a STL file"), ".",
        tr("STL file: *.stl (*.stl)"));
        
    vtkSTLWriter *writer = vtkSTLWriter::New();
    writer->SetFileName(fileName.toAscii().data());
    writer->SetInput(outputMesh);
    writer->Update();
    writer->Delete();
}


void PVolumeSegmenter::saveVolumeView()
{
    saveView(3);
}


void PVolumeSegmenter::saveMeshView()
{
    saveView(4);
}


void PVolumeSegmenter::saveView(int type)
{
    if (!loaded)
        return;
    
    if (type == 3 && !hasVolumeActor)
    {
        QMessageBox::critical(this, appName,
            "Please apply volume rendering first.");
        return;
    }
    else if (type == 4 && !hasMeshActor)
    {
        QMessageBox::critical(this, appName,
            "Please apply mesh generation first.");
        return;
    }  
        
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save current view to a file"), writeDir,
        tr("Image files: *.jpg, *.png, *.tif (*.jpg *.png *.tif)"));
        
    if (!fileName.isEmpty())
    {
        if (saveView(fileName, type))
            writeDir = QFileInfo(fileName).path();
    }
}


void PVolumeSegmenter::info()
{
    QString msg, msg2;
    
    if (!loaded)
        msg = QString("No volume loaded.");
    else if (reader)
    {       
        msg = QString("Source: %1<br><br>").arg(sourceName);
        
        vtkDICOMImageReader *dr = vtkDICOMImageReader::SafeDownCast(reader);
        vtkMetaImageReader *mr = vtkMetaImageReader::SafeDownCast(reader);
        
        if (dr)
        {
            double *sp = dr->GetPixelSpacing();
            msg += QString ("DICOM file<br><br>") +
            QString("Patient name: %1<br>").arg(dr->GetPatientName()) +
            QString("Description: %1<br>").arg(dr->GetDescriptiveName()) +
            QString("Study ID: %1<br>").arg(dr->GetStudyID()) +
            QString("Study UID: %1<br>").arg(dr->GetStudyUID()) +
            QString("Size: %1 x %2 x %3 voxels<br>").arg(imageWidth).
                arg(imageHeight).arg(imageDepth) +
            QString("Voxel spacing: %1, %2, %3<br>").arg(sp[0]).
                arg(sp[1]).arg(sp[2]);
        }
        else if (mr)
        {
            double *sp = mr->GetPixelSpacing();
            msg += QString ("MetaImage file<br><br>") +
            QString("Patient name: %1<br>").arg(mr->GetPatientName()) +
            QString("Description: %1<br>").arg(mr->GetDescriptiveName()) +
            QString("Study ID: %1<br>").arg(mr->GetStudyID()) +
            QString("Study UID: %1<br>").arg(mr->GetStudyUID()) +
            QString("Size: %1 x %2 x %3 voxels<br>").arg(imageWidth).
                arg(imageHeight).arg(imageDepth) +
            QString("Voxel spacing: %1, %2, %3<br>").arg(sp[0]).
                arg(sp[1]).arg(sp[2]);
        }
            
#ifdef DEBUG
        if (dr)
        {
            msg +=
            QString("<br>Bits allocated: %1<br>").
                arg(dr->GetBitsAllocated()) +
            QString("Number of components: %1<br>").
                arg(dr->GetNumberOfComponents()) +
            QString("Value: %1<br>").arg((dr->GetPixelRepresentation()
                ? "signed" : "unsigned")) +
            QString("Rescale slope: %1<br>").
                arg(dr->GetRescaleSlope()) +
            QString("Rescale offset: %1<br>").
                arg(dr->GetRescaleOffset());
        }
        else if (mr)
        {
            msg +=
            QString("<br>Bits allocated: %1<br>").
                arg(mr->GetBitsAllocated()) +
            QString("Number of components: %1<br>").
                arg(mr->GetNumberOfComponents()) +
            QString("Value: %1<br>").arg((mr->GetPixelRepresentation()
                ? "signed" : "unsigned")) +
            QString("Rescale slope: %1<br>").
                arg(mr->GetRescaleSlope()) +
            QString("Rescale offset: %1<br>").
                arg(mr->GetRescaleOffset());
        }
#endif
    }
        
    if (loaded && outputMesh)
    {
        vtkCellArray *mesh = outputMesh->GetPolys();
        msg2 = QString("<br>Mesh has %1 faces, %2 MB<br>").
           arg(mesh->GetNumberOfCells()).
           arg(mesh->GetActualMemorySize() / 1000.0, 0, 'f', 2);
    }
    else
        msg2 = QString();
           
    QMessageBox::about(this, tr("Data Information"),
        QString("<h2>%1</h2> <p></p>").arg(appName) + msg + msg2);
}


void PVolumeSegmenter::help()
{
    QMessageBox::about(this, tr("Help information"),
       QString("<h2>%1</h2>").arg(appName) +
       "<p></p>" +
       "<p>Tool Sequence:<br>" +
       "The tools, if used, are chained in the following order:<br>" +
       "1. Volume of interest (VOI) selection<br>" +
       "2. Thresholding<br>" +
       "3. Outupt generation: volume rendering or mesh<br>" +
       "Volume rendering is usually faster than mesh generation.<br>" +
       
       "<p>Control keys:<br>" +
       "left mouse button: camera rotate<br>" +
       "SHIFT + left mouse button: camera pan<br>" +
       "CTRL + left mouse button: camera spin<br>" +
       "CTRL + SHIFT + left mouse button: camera zoom<br>" +
       "middle mouse wheel: zoom<br>" +
       "middle mouse button: camera pan<br>" +
       "right mouse button: camera zoom<br>" +
       "r: reset camera<br>" +
       "i: toggle clipping box<br>");
}


void PVolumeSegmenter::resetPipeline()
{
    // Reset in reverse order
    
    if (meshRenderer)
    {
        // Remove renderer
        meshRenderer->RemoveAllViewProps();
        meshWidget->GetRenderWindow()->RemoveRenderer(meshRenderer);
        meshRenderer->Delete();
        
        // Re-create renderer
        meshRenderer = vtkRenderer::New();  // Camera is also reset.
        meshRenderer->SetBackground(0.0, 0.0, 0.0);
        meshWidget->GetRenderWindow()->AddRenderer(meshRenderer);
        hasMeshActor = false;
    }
    
    genMeshDialog->hide();
    mcubes->SetInputConnection(NULL);
    decimate->SetInputConnection(NULL);
    normals->SetInputConnection(NULL);
    genMeshAction->setChecked(false);
    
    if (volumeRenderer)
    {
        // Remove renderer
        volumeRenderer->RemoveAllViewProps();
        volumeWidget->GetRenderWindow()->RemoveRenderer(volumeRenderer);
        volumeRenderer->Delete();
        
        // Re-create renderer
        volumeRenderer = vtkRenderer::New();  // Camera is also reset.
        volumeRenderer->SetBackground(0.0, 0.0, 0.0);
        volumeWidget->GetRenderWindow()->AddRenderer(volumeRenderer);
        hasVolumeActor = false;
    }
    
    volumeRenderDialog->hide();
    volumeRenderAction->setChecked(false);
    
    if (setVoiAction->isChecked())  // Reset this one first
    {
        transVoi->hide();
        coronalVoi->hide();
        sagittalVoi->hide();
        // extractVoi->SetInputConnection(NULL);  // Don't set to NULL.
        setVoiAction->setChecked(false);
        voiDone = false;
    }
      
    thresholder->hide();
    thresholder->setInputConnection(NULL);
    thresholdAction->setChecked(false);
    thresholdDone = false;
    segmented = false;
}


void PVolumeSegmenter::resetInput()
{
    resetPipeline();
    
    if (volumeWidget->isVisible())
        volumeWidget->GetRenderWindow()->Render();
    else if (meshWidget->isVisible())
        meshWidget->GetRenderWindow()->Render();
    
    if (input)
    {
        outputFilter->SetInputConnection(input);
        transViewer->SetInputConnection(outputFilter->GetOutputPort());
        coronalViewer->SetInputConnection(outputFilter->GetOutputPort());
        sagittalViewer->SetInputConnection(outputFilter->GetOutputPort());
        
        transVoi->setInteractor(transViewer->GetRenderWindow()->
            GetInteractor());
        coronalVoi->setInteractor(coronalViewer->GetRenderWindow()->
            GetInteractor());
        sagittalVoi->setInteractor(sagittalViewer->GetRenderWindow()->
            GetInteractor());

        updateViewers();
        
        // Set initial VOI
        vtkImageAlgorithm *algo =
            vtkImageAlgorithm::SafeDownCast(input->GetProducer());
        int *bound = algo->GetOutput()->GetExtent();
        double *spacing = algo->GetOutput()->GetSpacing();
        selectedVoi[0] = bound[0] / spacing[0];
        selectedVoi[1] = bound[1] / spacing[0];
        selectedVoi[2] = bound[2] / spacing[1];
        selectedVoi[3] = bound[3] / spacing[1];
        selectedVoi[4] = bound[4] / spacing[2];
        selectedVoi[5] = bound[5] / spacing[2];
    } 
}


void PVolumeSegmenter::resetProcess()
{
    // Reset in reverse order
    genMeshDialog->hide();
    genMeshAction->setChecked(false);
    
    volumeRenderDialog->hide();
    volumeRenderAction->setChecked(false);
    
    thresholder->hide();
    thresholdAction->setChecked(false);
    thresholdDone = false;
    segmented = false;
}


void PVolumeSegmenter::resetVoi()
{
    if (loaded)
    {
        transVoi->initPosition(transViewer->GetImageActor(),
            transViewer->GetRenderer()->GetActiveCamera(), 10);
        coronalVoi->initPosition(coronalViewer->GetImageActor(),
            coronalViewer->GetRenderer()->GetActiveCamera(), 10);
        sagittalVoi->initPosition(sagittalViewer->GetImageActor(),
            sagittalViewer->GetRenderer()->GetActiveCamera(), 10);
        
        setVoiAction->setChecked(false);
        voiDone = false;
    }
}


void PVolumeSegmenter::setVoi(bool on)
{
    if (!loaded)
    {
        setVoiAction->setChecked(false);
        return;
    }
    
    transVoi->show(on);
    coronalVoi->show(on);
    sagittalVoi->show(on);
    transVoi->update();
    coronalVoi->update();
    sagittalVoi->update();
    updateViewers();
}


void PVolumeSegmenter::cropVoi()
{   
    vtkImageAlgorithm *algo =
        vtkImageAlgorithm::SafeDownCast(input->GetProducer());
    double *spacing = algo->GetOutput()->GetSpacing();
    double *bound = computeBounds();
    selectedVoi[0] = bound[0] / spacing[0];
    selectedVoi[1] = bound[1] / spacing[0];
    selectedVoi[2] = bound[2] / spacing[1];
    selectedVoi[3] = bound[3] / spacing[1];
    selectedVoi[4] = bound[4] / spacing[2];
    selectedVoi[5] = bound[5] / spacing[2];
    
    extractVoi->SetVOI(selectedVoi);
    extractVoi->SetInputConnection(input);
    outputFilter->SetInputConnection(extractVoi->GetOutputPort());
    updateViewers();
    voiDone = true;
    
    emit voiChanged();
}


void PVolumeSegmenter::showThresholder()
{    
    if (!loaded)
    {
        QMessageBox::critical(this, appName,
            "No volume image to work on.<br>Please load a volume image.");
        thresholdAction->setChecked(false);
        return;
    }
    
    bool visible = thresholdAction->isChecked();
    thresholder->setVisible(visible);

    if (visible)
    {   
        if (voiDone)
            thresholder->setInputConnection(
                extractVoi->GetOutputPort());
        else
            thresholder->setInputConnection(input);
            
        outputFilter->SetInputConnection(thresholder->getOutputPort());
        thresholdDone = true;
        segmented = true;
    }
}


void PVolumeSegmenter::showVolumeRenderDialog()
{
    if (!loaded)
    {
        QMessageBox::critical(this, appName,
            "No volume image to work on.<br>Please load a volume image.");
        volumeRenderAction->setChecked(false);
        return;
    }
    
    if (meshWidget->isVisible())  // Changing from mesh view
    {
        meshWidget->hide();
        volumeWidget->show();
    }
    
    bool visible = volumeRenderAction->isChecked();
    volumeRenderDialog->setVisible(visible);
}


void PVolumeSegmenter::volumeRender()
{
    if (!loaded)
    {
        QMessageBox::critical(this, appName,
            "No volume image to work on.<br>Please load a volume image.");
        volumeRenderAction->setChecked(false);
        return;
    }
    
    if (!hasVolumeActor)
    {
        volumeRenderer->AddActor(volumeActor);
        hasVolumeActor = true;
    }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    computeOutputVolume();
    setBlendType();
#ifdef USE_SMART_MAPPER
    if (sampleDistanceBox->currentIndex() == 0)
    {
        smartMapper->SetInput(outputVolume);
        volumeMapper = smartMapper;
    }
    else
    {
        rayCastMapper->SetInput(outputVolume);
        volumeMapper = rayCastMapper;
    }
#else
    rayCastMapper->SetInput(outputVolume);
    volumeMapper = rayCastMapper;
#endif
    volumeActor->SetMapper(volumeMapper);
    volumeRenderer->ResetCamera();
    volumeWidget->GetRenderWindow()->Render();
    QApplication::restoreOverrideCursor();
}


void PVolumeSegmenter::setSampleDistance(int option)
{
    if (!loaded)
        return;
       
    float distance = 1.0 / (1 << option);
    rayCastMapper->SetSampleDistance(distance);
}


void PVolumeSegmenter::showGenMeshDialog()
{    
    if (!loaded)
    {
        QMessageBox::critical(this, appName,
            "No volume image to work on.<br>Please load a volume image.");
        genMeshAction->setChecked(false);
        return;
    }
    
    if (!segmented)
    {
        QMessageBox::critical(this, appName,
            "Please apply segmentation tools first.");
        genMeshAction->setChecked(false);
        return;
    }
    
    if (volumeWidget->isVisible())  // Changing from volume rendering view
    {
        volumeWidget->hide();
        meshWidget->show();
    }
    
    bool visible = genMeshAction->isChecked();
    genMeshDialog->setVisible(visible);
}


void PVolumeSegmenter::generateMesh()
{
    if (!loaded)
    {
        QMessageBox::critical(this, appName,
            "No volume image to work on.<br>Please load a volume image.");
        return;
    }
    
    if (!segmented)
    {
        QMessageBox::critical(this, appName,
            "Please apply segmentation first.");
        return;
    }
    
    if (!hasMeshActor)
    {
        meshRenderer->AddActor(meshActor);
        hasMeshActor = true;
    }
    
    mcubes->SetInputConnection(outputFilter->GetOutputPort());
    mcubes->SetValue(0, intensityBox->value());
    normals->SetInputConnection(mcubes->GetOutputPort());
        
    QApplication::setOverrideCursor(Qt::WaitCursor);
    normals->UpdateWholeExtent();
    outputMesh->DeepCopy(normals->GetOutput());
    outputMesh->Update();
    
    meshMapper->SetInput(outputMesh);  // Cut connection from normals.
    meshActor->SetMapper(meshMapper);
    meshRenderer->ResetCamera();
    meshWidget->GetRenderWindow()->Render();
    QApplication::restoreOverrideCursor();
}


// Mesh decimation and smoothing

void PVolumeSegmenter::smoothing()
{
    if (!outputMesh)
    {
        QMessageBox::critical(this, appName,
            "Please generate the segmented mesh first.");
        return;
    }
    
    decimate->SetInputConnection(mcubes->GetOutputPort());
    decimate->SetTargetReduction(1.0 - ratioBox->value());
    smooth->SetRelaxationFactor(factorBox->value());
    smooth->SetNumberOfIterations(smoothIterBox->value());
    // decimate to smooth is already connected.
    normals->SetInputConnection(smooth->GetOutputPort());
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    normals->UpdateWholeExtent();
    outputMesh->DeepCopy(normals->GetOutput());
    outputMesh->Update();
    
    meshMapper->SetInput(outputMesh);  // Cut connection from normals.
    meshWidget->GetRenderWindow()->Render();
    QApplication::restoreOverrideCursor();
}


// Supporting functions

bool PVolumeSegmenter::saveToFile(const QString &fileName)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    computeOutputVolume();
    QApplication::restoreOverrideCursor();
    
    vtkMetaImageWriter *writer = vtkMetaImageWriter::New();
    writer->SetFileName(fileName.toAscii().data());
    writer->SetCompression(true);
    writer->SetInput(outputVolume);
    writer->Write();
    writer->Delete();
    return true;
}


bool PVolumeSegmenter::saveView(const QString &fileName, int type)
{
    vtkWindowToImageFilter *filter = vtkWindowToImageFilter::New();
    switch (type)
    {            
        case 0:
            filter->SetInput(transWidget->GetRenderWindow());
            break;
            
        case 1:
            filter->SetInput(coronalWidget->GetRenderWindow());
            break;
            
        case 2:
            filter->SetInput(sagittalWidget->GetRenderWindow());
            break;
            
        case 3:
            filter->SetInput(volumeWidget->GetRenderWindow());
            break;
            
        case 4:
            filter->SetInput(meshWidget->GetRenderWindow());
            break;
            
        default:
            return false;
    }

    QString suffix = QFileInfo(fileName).suffix();
    vtkImageWriter *writer;

    if (suffix == "jpg")
        writer = vtkJPEGWriter::New();
    else if (suffix == "png")
        writer = vtkPNGWriter::New();
    else if (suffix == "tif")
        writer = vtkTIFFWriter::New();
    else
    {
        QMessageBox::critical(this, appName,
            QString("File type %1 is unsupported.").arg(suffix));
        return false;
    }

    writer->SetInput(filter->GetOutput());
    writer->SetFileName(fileName.toAscii().data());
    writer->Write();
    writer->Delete();
    filter->Delete();
    return true;
}


double *PVolumeSegmenter::computeBounds()
{
    static double bound[6];  // min x, max x, min y, max y, min z, max z
    double *b;

    b = transVoi->getBounds();  // XY plane
    bound[0] = b[0];
    bound[1] = b[1];
    bound[2] = b[2];
    bound[3] = b[3];
    
    b = coronalVoi->getBounds();  // XZ plane
    bound[0] = qMin(bound[0], b[0]);
    bound[1] = qMax(bound[1], b[1]);
    bound[4] = b[2];
    bound[5] = b[3];
    
    b = sagittalVoi->getBounds();  // YZ plane
    bound[2] = qMin(bound[2], b[0]);
    bound[3] = qMax(bound[3], b[1]);
    bound[4] = qMin(bound[4], b[2]);
    bound[5] = qMax(bound[5], b[3]);
    
    return bound;
}


void PVolumeSegmenter::computeOutputVolume()
{
    outputFilter->UpdateWholeExtent();
    outputVolume->DeepCopy(outputFilter->GetOutput());
    outputVolume->Update();
}


void PVolumeSegmenter::setBlendType()
{       
    // Init
    opacityFn->RemoveAllPoints();
    colorFn->RemoveAllPoints();
    
    int blendType = viewTypeBox->currentIndex();
    double ambient = ambientBox->value();
    double specular = specularBox->value();
    
    // Add function points
    switch (blendType)
    {
        // CT Skin
        // Use compositing and functions set to highlight skin in CT data.
        case 0:     
            opacityFn->AddPoint(-2000, 0.0, 0.5, 0.0);
            opacityFn->AddPoint( -300, 0.0, 0.5, 0.5);
            opacityFn->AddPoint( -100, 1.0, 0.5, 0.0);
            opacityFn->AddPoint( 2000, 1.0, 0.5, 0.0);
                  
            colorFn->AddRGBPoint(-2000, 0.0, 0.0, 0.0, 0.5, 0.0);
            colorFn->AddRGBPoint( -300, 0.6, 0.4, 0.2, 0.5, 0.5);
            colorFn->AddRGBPoint( -100, 0.9, 0.8, 0.5, 0.5, 0.0);
            colorFn->AddRGBPoint( 2000, 1.0, 1.0, 1.0, 0.5, 0.0);
  
            volumeMapper->SetBlendModeToComposite();
            property->ShadeOn();
            property->SetAmbient(ambient);
            property->SetDiffuse(0.9);
            property->SetSpecular(specular);
            property->SetSpecularPower(10.0);
            break;
  
        // CT Muscle
        // Use compositing and functions set to highlight muscle in CT data.
        case 1:     
            opacityFn->AddPoint(-2000, 0.0, 0.5, 0.0);
            opacityFn->AddPoint(  -70, 0.0, 0.5, 0.5);
            opacityFn->AddPoint(  200, 1.0, 0.5, 0.0);
            opacityFn->AddPoint( 2000, 1.0, 0.5, 0.0);
                  
            colorFn->AddRGBPoint(-2000, 0.0, 0.0, 0.0, 0.5, 0.0);
            colorFn->AddRGBPoint(  -70, 1.0, 0.6, 0.6, 0.5, 0.5);
            colorFn->AddRGBPoint(  200, 1.0, 1.0, 1.0, 0.5, 0.0);
            colorFn->AddRGBPoint( 2000, 1.0, 1.0, 1.0, 0.5, 0.0);

            volumeMapper->SetBlendModeToComposite();
            property->ShadeOn();
            property->SetAmbient(ambient);
            property->SetDiffuse(0.9);
            property->SetSpecular(specular);
            property->SetSpecularPower(10.0);
            break;
            
        // CT Bone
        // Use compositing and functions set to highlight bone in CT data.
        case 2:
            opacityFn->AddPoint(-2000, 0.0, 0.5, 0.0);
            opacityFn->AddPoint(  -20, 0.0, 0.5, 0.5);
            opacityFn->AddPoint(  300, 1.0, 0.5, 0.0);
            opacityFn->AddPoint( 2000, 1.0, 0.5, 0.0);

            colorFn->AddRGBPoint(-2000, 0.0, 0.0, 0.0, 0.5, 0.0);
            colorFn->AddRGBPoint(  -20, 1.0, 0.3, 0.3, 0.5, 0.5);
            colorFn->AddRGBPoint(  300, 1.0, 1.0, 1.0, 0.5, 0.0);
            colorFn->AddRGBPoint( 2000, 1.0, 1.0, 1.0, 0.5, 0.0);
  
            volumeMapper->SetBlendModeToComposite();
            property->ShadeOn();
            property->SetAmbient(ambient);
            property->SetDiffuse(0.9);
            property->SetSpecular(specular);
            property->SetSpecularPower(10.0);
            break;
            
        // CTA Vessels
        // Use compositing and functions set to highlight bone in CT data.
        case 3:
            opacityFn->AddPoint(-2000, 0.0, 0.5, 0.0);
            opacityFn->AddPoint(  -20, 0.0, 0.5, 0.5);
            opacityFn->AddPoint(   50, 0.0, 0.5, 0.5);
            opacityFn->AddPoint(  100, 1.0, 0.5, 0.0);
            opacityFn->AddPoint( 2000, 1.0, 0.5, 0.0);

            colorFn->AddRGBPoint(-2000, 0.0, 0.0, 0.0, 0.5, 0.0);
            colorFn->AddRGBPoint(  -20, 0.3, 0.3, 0.3, 0.5, 0.5);
            colorFn->AddRGBPoint(   50, 0.3, 0.1, 0.1, 0.5, 0.5);
            colorFn->AddRGBPoint(  100, 1.0, 0.6, 0.6, 0.5, 0.0);
            colorFn->AddRGBPoint( 2000, 1.0, 0.6, 0.6, 0.5, 0.0);
  
            volumeMapper->SetBlendModeToComposite();
            property->ShadeOn();
            property->SetAmbient(ambient);
            property->SetDiffuse(0.9);
            property->SetSpecular(specular);
            property->SetSpecularPower(10.0);
            break;
  
        default: // Case 0
            opacityFn->AddPoint(-2000, 0.0, 0.5, 0.0);
            opacityFn->AddPoint( -300, 0.0, 0.5, 0.5);
            opacityFn->AddPoint( -100, 1.0, 0.5, 0.0);
            opacityFn->AddPoint( 2000, 1.0, 0.5, 0.0);
                  
            colorFn->AddRGBPoint(-2000, 0.0, 0.0, 0.0, 0.5, 0.0);
            colorFn->AddRGBPoint( -300, 0.6, 0.4, 0.2, 0.5, 0.5);
            colorFn->AddRGBPoint( -100, 0.9, 0.7, 0.3, 0.5, 0.0);
            colorFn->AddRGBPoint( 2000, 1.0, 1.0, 1.0, 0.5, 0.0);
  
            volumeMapper->SetBlendModeToComposite();
            property->ShadeOn();
            property->SetAmbient(ambient);
            property->SetDiffuse(0.9);
            property->SetSpecular(specular);
            property->SetSpecularPower(10.0);
            break;
    }
}
