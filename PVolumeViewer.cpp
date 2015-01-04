/* PVolumeViewer.h

   3-plane volume viewer.

   Copyright 2012, 2103, 2014 National University of Singapore
   Author: Leow Wee Kheng
*/


#include "PVolumeViewer.h"
#include <QtGui>

#include "vtkCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkWindowToImageFilter.h"

#include "vtkDICOMImageReader.h"
#include "vtkMetaImageReader.h"
#include "vtkMetaImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkTIFFWriter.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkPlanes.h"
#include "vtkProperty.h"
#include "vtkPointData.h"
#include "vtkImageActor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkAssemblyPath.h"
#include "vtkCell.h"
#include "vtkExtractVOI.h"

using namespace std;

#define DEBUG


// Callback class

class PVolumeViewerCallback: public vtkCommand
{
public:
    static PVolumeViewerCallback *New() { return new PVolumeViewerCallback; }
    void Execute(vtkObject *caller, unsigned long eventId, void *callData);
    PVolumeViewer *dview;
    vtkImageViewer2 *viewer;

protected:
    PVolumeViewerCallback();
    ~PVolumeViewerCallback();
    
private:
    vtkPointData *pointData;
};


//----- PVolumeViewer class ----------

PVolumeViewer::PVolumeViewer()
{
    // Initialisation
    reader = NULL;
    input = NULL;
    transViewer = NULL;
    transWidget = NULL;
    coronalViewer = NULL;
    coronalWidget = NULL;
    sagittalViewer = NULL;
    sagittalWidget = NULL;
    picker = NULL;
    
    appName = QString("Volume Viewer");
    readDir = ".";
    writeDir = ".";
    numPane = 4;
    loaded = false;
    crossHairOn = false;
    
    // Create GUI
    createWidgets();
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
}


PVolumeViewer::~PVolumeViewer()
{
    uninstallPipeline();
    
    transXHair->Delete();
    transXMapper->Delete();
    transXActor->Delete();
}


void PVolumeViewer::setAppName(const QString &name)
{
    appName = name;
}


// Qt event handlers

void PVolumeViewer::closeEvent(QCloseEvent *event)
{
    event->accept();
}


//----- Create window widgets ----------

void PVolumeViewer::createWidgets()
{    
    // Create volume viewer widgets for transverse view
    transWidget = new QVTKWidget;
    transWidget->GetRenderWindow();
    transSlider = new QSlider(Qt::Vertical);
    transSlider->setRange(0, 1);
    transSlider->setValue(0);
    transSlider->setInvertedAppearance(true);
    transSlider->setInvertedControls(true);
    connect(transSlider, SIGNAL(valueChanged(int)), this,
        SLOT(transViewSetSlice(int)));
    QHBoxLayout *transBox = new QHBoxLayout;
    transBox->addWidget(transWidget);
    transBox->addWidget(transSlider);
    pane00 = new QWidget;
    pane00->setLayout(transBox);
    
    // Unused pane
    pane01 = new QWidget;
    
    // Create volume viewer widgets for coronal view
    coronalWidget = new QVTKWidget;
    coronalWidget->GetRenderWindow();
    coronalSlider = new QSlider(Qt::Vertical);
    coronalSlider->setRange(0, 1);
    coronalSlider->setValue(0);
    coronalSlider->setInvertedAppearance(true);
    coronalSlider->setInvertedControls(true);
    connect(coronalSlider, SIGNAL(valueChanged(int)), this,
        SLOT(coronalViewSetSlice(int)));
    QHBoxLayout *coronalBox = new QHBoxLayout;
    coronalBox->addWidget(coronalWidget);
    coronalBox->addWidget(coronalSlider);
    pane10 = new QWidget;
    pane10->setLayout(coronalBox);
    
    // Create volume viewer widget for sagittal view
    sagittalWidget = new QVTKWidget;
    sagittalWidget->GetRenderWindow();
    sagittalSlider = new QSlider(Qt::Vertical);
    sagittalSlider->setRange(0, 1);
    sagittalSlider->setValue(0);
    sagittalSlider->setInvertedAppearance(true);
    sagittalSlider->setInvertedControls(true);
    connect(sagittalSlider, SIGNAL(valueChanged(int)), this,
        SLOT(sagittalViewSetSlice(int)));
    QHBoxLayout *sagittalBox = new QHBoxLayout;
    sagittalBox->addWidget(sagittalWidget);
    sagittalBox->addWidget(sagittalSlider);
    pane11 = new QWidget;
    pane11->setLayout(sagittalBox);

    // Layout
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(pane00, 0, 0);
    grid->addWidget(pane01, 0, 1);
    grid->addWidget(pane10, 1, 0);
    grid->addWidget(pane11, 1, 1);
    QWidget *main = new QWidget;
    main->setLayout(grid);
    setCentralWidget(main); 
    
    // Create combo boxes        
    windowLevelBox = new QComboBox(this);
    QStringList choices;
    choices.clear();
    choices << "Default" << "CT Abdomen" << "CT Brain" << "CT Bone" 
        << "CT Bone details" << "CT Head" << "CT Skin";
    windowLevelBox->insertItems(0, choices);
    connect(windowLevelBox, SIGNAL(activated(int)), this,
        SLOT(setWindowLevel(int)));
        
    // Overall
    setWindowTitle(appName);
    setWindowIcon(QIcon(":/images/panax-icon.png"));

    winWidth = 750;
    winHeight = 710;
    setMinimumSize(winWidth, winHeight); 
    transWidget->updateGeometry();
    coronalWidget->updateGeometry();
    sagittalWidget->updateGeometry();
    
    // Others
    createCrossHairs();
}


#define LowBound -2000
#define HighBound 2000

void PVolumeViewer::createCrossHairs()
{
    transXHair = vtkCursor3D::New();
    transXHair->SetModelBounds(LowBound, HighBound, LowBound, HighBound,
        LowBound, HighBound);
    transXHair->AllOff();
    transXMapper = vtkPolyDataMapper::New();
    transXMapper->SetInputConnection(transXHair->GetOutputPort());
    transXActor = vtkActor::New();
    transXActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    transXActor->SetMapper(transXMapper);
    
    coronalXHair = vtkCursor3D::New();
    coronalXHair->SetModelBounds(LowBound, HighBound, LowBound, HighBound,
        LowBound, HighBound);
    coronalXHair->AllOff();
    coronalXMapper = vtkPolyDataMapper::New();
    coronalXMapper->SetInputConnection(coronalXHair->GetOutputPort());
    coronalXActor = vtkActor::New();
    coronalXActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    coronalXActor->SetMapper(coronalXMapper);
    
    sagittalXHair = vtkCursor3D::New();
    sagittalXHair->SetModelBounds(LowBound, HighBound, LowBound, HighBound,
        LowBound, HighBound);
    sagittalXHair->AllOff();
    sagittalXMapper = vtkPolyDataMapper::New();
    sagittalXMapper->SetInputConnection(sagittalXHair->GetOutputPort());
    sagittalXActor = vtkActor::New();
    sagittalXActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    sagittalXActor->SetMapper(sagittalXMapper);
}


void PVolumeViewer::createActions()
{
    loadDirAction = new QAction(tr("Load from &Directory"), this);
    loadDirAction->setIcon(QIcon(":/images/loaddir.png"));
    loadDirAction->setShortcut(tr("Ctrl+D"));
    loadDirAction->setStatusTip(tr("Load DICOM volume in directory."));
    connect(loadDirAction, SIGNAL(triggered()), this, SLOT(loadDir()));
    connect(loadDirAction, SIGNAL(triggered()),
        this, SLOT(resetCrossHair()));
    
    loadFileAction = new QAction(tr("Load &File"), this);
    loadFileAction->setIcon(QIcon(":/images/open.png"));
    loadFileAction->setShortcut(tr("Ctrl+F"));
    loadFileAction->setStatusTip(tr("Load volume image from a file."));
    connect(loadFileAction, SIGNAL(triggered()), this, SLOT(loadFile()));
    connect(loadFileAction, SIGNAL(triggered()),
        this, SLOT(resetCrossHair()));
        
    saveFileAction = new QAction(tr("&Save File"), this);
    saveFileAction->setIcon(QIcon(":/images/save.png"));
    saveFileAction->setShortcut(tr("Ctrl+S"));
    saveFileAction->setStatusTip(tr("Save volume image into a file."));
    connect(saveFileAction, SIGNAL(triggered()), this, SLOT(saveFile()));
        
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit Mesh Viewer"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    
    setViewPaneAction = new QAction(tr("Set viewing &Panes"), this);
    setViewPaneAction->setIcon(QIcon(":/images/viewpane.png"));
    setViewPaneAction->setShortcut(tr("Ctrl+P"));
    setViewPaneAction->setStatusTip(tr("Set number of viewing panes"));
    connect(setViewPaneAction, SIGNAL(triggered()),
        this, SLOT(setViewPane()));
        
    setCrossHairAction = new QAction(tr("Set &Cross Hair"), this);
    setCrossHairAction->setIcon(QIcon(":/images/crosshair.png"));
    setCrossHairAction->setStatusTip(tr("Set cross hair."));
    setCrossHairAction->setCheckable(true);
    connect(setCrossHairAction, SIGNAL(toggled(bool)),
        this, SLOT(setCrossHair(bool)));
        
    resetZoomAction = new QAction(tr("Reset &Zoom"), this);
    resetZoomAction->setIcon(QIcon(":/images/reset2.png"));
    resetZoomAction->setShortcut(tr("Ctrl+Z"));
    resetZoomAction->setStatusTip(tr("Reset zoom."));
    connect(resetZoomAction, SIGNAL(triggered()),
        this, SLOT(resetZoom()));
    
    resetWindowLevelAction = new QAction(tr("Reset &Window Level"), this);
    resetWindowLevelAction->setIcon(QIcon(":/images/contrast.png"));
    resetWindowLevelAction->setShortcut(tr("Ctrl+W"));
    resetWindowLevelAction->setStatusTip(tr("Reset window and level"));
    connect(resetWindowLevelAction, SIGNAL(triggered()),
        this, SLOT(resetWindowLevel()));
               
    saveTransViewAction = new QAction(tr("&Transverse view"), this);
    saveTransViewAction->setStatusTip(tr("Save transverse view to a file"));
    connect(saveTransViewAction, SIGNAL(triggered()),
        this, SLOT(saveTransView()));
        
    saveCoronalViewAction = new QAction(tr("&Coronal view"), this);
    saveCoronalViewAction->setStatusTip(tr("Save coronal view to a file"));
    connect(saveCoronalViewAction, SIGNAL(triggered()),
        this, SLOT(saveCoronalView()));
        
    saveSagittalViewAction = new QAction(tr("&Sagittal view"), this);
    saveSagittalViewAction->setStatusTip(tr("Save sagittal view to a file"));
    connect(saveSagittalViewAction, SIGNAL(triggered()),
        this, SLOT(saveSagittalView()));
        
    infoAction = new QAction(tr("&Info"), this);
    infoAction->setIcon(QIcon(":/images/info.png"));
    infoAction->setShortcut(tr("Ctrl+I"));
    infoAction->setStatusTip(tr("Data information"));
    connect(infoAction, SIGNAL(triggered()), this, SLOT(info()));
        
    helpAction = new QAction(tr("&Help"), this);
    helpAction->setIcon(QIcon(":/images/help.png"));
    helpAction->setShortcut(tr("Ctrl+H"));
    helpAction->setStatusTip(tr("Help information"));
    connect(helpAction, SIGNAL(triggered()), this, SLOT(help()));
        
    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setShortcut(tr("Ctrl+A"));
    aboutAction->setStatusTip(tr("About this viewer"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
}


void PVolumeViewer::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(loadDirAction);
    fileMenu->addAction(loadFileAction);
    fileMenu->addAction(saveFileAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(setViewPaneAction);
    viewMenu->addAction(setCrossHairAction);
    viewMenu->addAction(resetZoomAction);
    viewMenu->addAction(resetWindowLevelAction);
    
    saveViewMenu = viewMenu->addMenu(tr("&Save"));
    saveViewMenu->setIcon(QIcon(":/images/saveview.png"));
    saveViewMenu->addAction(saveTransViewAction);
    saveViewMenu->addAction(saveCoronalViewAction);
    saveViewMenu->addAction(saveSagittalViewAction);
    viewMenu->addAction(infoAction);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(helpAction);
    helpMenu->addAction(aboutAction);
}


void PVolumeViewer::createToolBars()
{
    fileToolBar = addToolBar(tr("&File"));
    fileToolBar->addAction(loadDirAction);
    fileToolBar->addAction(loadFileAction);
    fileToolBar->addAction(saveFileAction);
    
    viewToolBar = addToolBar(tr("&View"));
    viewToolBar->addAction(setViewPaneAction);
    viewToolBar->addAction(setCrossHairAction);
    viewToolBar->addAction(resetZoomAction);
    viewToolBar->addAction(resetWindowLevelAction);
    viewToolBar->addWidget(windowLevelBox);
    viewToolBar->addAction(infoAction);
    
    helpToolBar = addToolBar(tr("&Help"));
    helpToolBar->addAction(helpAction);
}


void PVolumeViewer::createStatusBar()
{
    statusBar()->showMessage(tr(""));
}


//----- Slot functions ----------

void PVolumeViewer::loadDir()
{
    QString	dirName = QFileDialog::getExistingDirectory(this, 
        tr("Load DICOM volume by directory"), readDir);
        
    if (!dirName.isEmpty())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        if (loadFromDir(dirName))
            readDir = dirName;
        QApplication::restoreOverrideCursor();
    }
}


void PVolumeViewer::loadFile()
{
    QString	fileName = QFileDialog::getOpenFileName(this, 
        tr("Load volume image from a file"), readDir,
        tr("Volume image: *.mha, *.mhd (*.mha *.mhd)"));
        
    if (!fileName.isEmpty())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        if (loadFromFile(fileName))
            readDir = QFileInfo(fileName).path();
        QApplication::restoreOverrideCursor();
    }
}


void PVolumeViewer::saveFile()
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


void PVolumeViewer::setViewPane()
{
    if (numPane == 1)
    {
        pane01->show();
        pane10->hide();
        pane11->hide();
        numPane = 2;
    }
    else if (numPane == 2)
    {
        pane10->show();
        pane11->show();
        numPane = 4;
    }
    else if (numPane == 4)
    {
        pane01->hide();
        pane10->hide();
        pane11->hide();
        numPane = 1;
    }
}


void PVolumeViewer::resetZoom()
{
    if (!loaded)
        return;
        
    setCrossHairAction->setChecked(false);
    transViewer->GetRenderer()->ResetCamera();
    coronalViewer->GetRenderer()->ResetCamera();
    sagittalViewer->GetRenderer()->ResetCamera();
    updateViewers();
}


void PVolumeViewer::resetWindowLevel()
{
    if (!loaded)
        return;

    setWindowLevel(0);
    windowLevelBox->setCurrentIndex(0);
}


void PVolumeViewer::setWindowLevel(int index)
{
    int level, window;
    
    if (!loaded)
        return;

    switch (index)
    {
        case 0:  // Default
            level = 128;
            window = 256;
            break;

        case 1:  // Abdomen
            level = 90;
            window = 400;
            break;

        case 2:  // Brain
            level = 45;
            window = 90;
            break;

        case 3:  // Bone
            level = 128;
            window = 200;
            break;

        case 4:  // Bone details
            level = 0;
            window = 2500;
            break;
            
        case 5:  // Head
            level = 128;
            window = 256;
            break;

        case 6:  // Skin
            level = 0;
            window = 400;
            break;

        default:
            level = 128;
            window = 256;
            break;
    }

    setWindowLevel(window, level);
}


void PVolumeViewer::saveTransView()
{
    saveView(0);
}


void PVolumeViewer::saveCoronalView()
{
    saveView(1);
}


void PVolumeViewer::saveSagittalView()
{
    saveView(2);
}


void PVolumeViewer::saveView(int type)
{
    if (!loaded)
        return;
        
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save current view to a file"), writeDir,
        tr("Image files: *.jpg, *.png, *.tif (*.jpg *.png *.tif)"));
        
    if (fileName.isEmpty())
        return;

    if (saveView(fileName, type))
        writeDir = QFileInfo(fileName).path();
}


void PVolumeViewer::info()
{
    QString msg;
    
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
    else
        msg = "External Input<br>";
           
    QMessageBox::about(this, tr("Data Information"),
        QString("<h2>%1</h2> <p></p>").arg(appName) + msg);
}


void PVolumeViewer::help()
{
    QMessageBox::about(this, tr("Help information"),
       QString("<h2>%1</h2>").arg(appName) +
       "<p></p>" +
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


void PVolumeViewer::about()
{
    QMessageBox::about(this, QString("About %1").arg(appName),
       QString("<h2>%1</h2>").arg(appName) +
       "<p>Copyright &copy; 2012, 2013, 2014<br>" +
       "Leow Wee Kheng<br>" +
       "Department of Computer Science<br>" +
       "National University of Singapore<p></p>" +
       "Implemented using Qt, VTK.");
}


void PVolumeViewer::resetCrossHair()
{
    if (loaded)
    {
        setTransCrossHair();
        setCoronalCrossHair();
        setSagittalCrossHair();
        transViewer->GetRenderer()->AddActor(transXActor);
        coronalViewer->GetRenderer()->AddActor(coronalXActor);
        sagittalViewer->GetRenderer()->AddActor(sagittalXActor);
        setCrossHairAction->setChecked(false);     
    }
}


void PVolumeViewer::setCrossHair(bool on)
{
    if (!loaded)
    {
        setCrossHairAction->setChecked(false);
        return;
    }
    
    if (on)
    {
        transXHair->AxesOn();
        coronalXHair->AxesOn();
        sagittalXHair->AxesOn();
    }
    else
    {
        transXHair->AxesOff();
        coronalXHair->AxesOff();
        sagittalXHair->AxesOff();
    }
        
    updateViewers();
}


void PVolumeViewer::transViewSetSlice(int slice)
{
    if (!loaded)
        return;
        
    transSlice = slice;
    transViewer->SetSlice(slice);
    setCoronalCrossHair();
    setSagittalCrossHair();
    updateViewers();
    statusBar()->showMessage(QString("Slice:(%1, %2, %3)").
        arg(sagittalSlice).arg(coronalSlice).arg(transSlice));
}


void PVolumeViewer::coronalViewSetSlice(int slice)
{
    if (!loaded)
        return;
        
    coronalSlice = slice;
    coronalViewer->SetSlice(slice);
    setTransCrossHair();
    setSagittalCrossHair();
    updateViewers();
    statusBar()->showMessage(QString("Slice:(%1, %2, %3)").
        arg(sagittalSlice).arg(coronalSlice).arg(transSlice));
}


void PVolumeViewer::sagittalViewSetSlice(int slice)
{
    if (!loaded)
        return;
        
    sagittalSlice = slice;
    sagittalViewer->SetSlice(slice);
    setTransCrossHair();
    setCoronalCrossHair();
    updateViewers();
    statusBar()->showMessage(QString("Slice:(%1, %2, %3)").
        arg(sagittalSlice).arg(coronalSlice).arg(transSlice));
}


void PVolumeViewer::updateViewers()
{
    if (!loaded)
        return;
        
    transViewer->UpdateDisplayExtent();
    transViewer->Render();
    coronalViewer->UpdateDisplayExtent();
    coronalViewer->Render();
    sagittalViewer->UpdateDisplayExtent();
    sagittalViewer->Render();
}


//----- Supporting functions ----------

void PVolumeViewer::installPipeline()
{
    vtkRenderWindow *renderWindow;
    
    // Create transverse image viewer
    if (reader)
        input = reader->GetOutputPort();
    else
    {
        cout << "Error: In PVolumeViewer::installPipeline(): "
            << "Reader is NULL.\n" << flush;
        return;
    }

    transViewer = vtkImageViewer2::New();
    transViewer->SetSliceOrientationToXY();
    transViewer->SetInputConnection(input);
    transViewer->GetRenderer();  // Init renderer
    renderWindow = transViewer->GetRenderWindow();
    transWidget->SetRenderWindow(renderWindow);
    transViewer->SetupInteractor(renderWindow->GetInteractor());
    
    // Create coronal image viewer
    coronalViewer = vtkImageViewer2::New();
    coronalViewer->SetSliceOrientationToXZ();
    coronalViewer->SetInputConnection(input);
    coronalViewer->GetRenderer();
    renderWindow = coronalViewer->GetRenderWindow();
    coronalWidget->SetRenderWindow(renderWindow);
    coronalViewer->SetupInteractor(renderWindow->GetInteractor());
    
    // Create sgaittal image viewer
    sagittalViewer = vtkImageViewer2::New();
    sagittalViewer->SetSliceOrientationToYZ();
    sagittalViewer->SetInputConnection(input);
    sagittalViewer->GetRenderer();
    renderWindow = sagittalViewer->GetRenderWindow();
    sagittalWidget->SetRenderWindow(renderWindow);
    sagittalViewer->SetupInteractor(renderWindow->GetInteractor());
    
    loaded = true;
    
    // Sets up callback
    picker = vtkPropPicker::New();
    picker->PickFromListOn();
    picker->AddPickList(transViewer->GetImageActor());
    picker->AddPickList(coronalViewer->GetImageActor());
    picker->AddPickList(sagittalViewer->GetImageActor());

    PVolumeViewerCallback *callback;
    vtkInteractorStyleImage* style;
    
    callback = PVolumeViewerCallback::New();
    callback->dview = this;
    callback->viewer = transViewer;
    style = transViewer->GetInteractorStyle();
    style->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
    style->AddObserver(vtkCommand::MouseMoveEvent, callback);
    style->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);
    callback->Delete();
    
    callback = PVolumeViewerCallback::New();
    callback->dview = this;
    callback->viewer = coronalViewer;
    style = coronalViewer->GetInteractorStyle();
    style->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
    style->AddObserver(vtkCommand::MouseMoveEvent, callback);
    style->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);
    callback->Delete();
    
    callback = PVolumeViewerCallback::New();
    callback->dview = this;
    callback->viewer = sagittalViewer;
    style = sagittalViewer->GetInteractorStyle();
    style->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
    style->AddObserver(vtkCommand::MouseMoveEvent, callback);
    style->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);
    callback->Delete();
}


void PVolumeViewer::uninstallPipeline()
{   
    if (reader)
    {
        reader->Delete();
        reader = NULL;
    }
    
    input = NULL;
    
    if (transViewer)
    {
        transViewer->Delete();
        transViewer = NULL;
    }
    
    if (coronalViewer)
    {
        coronalViewer->Delete();
        coronalViewer = NULL;
    }
    
    if (sagittalViewer)
    {
        sagittalViewer->Delete();
        sagittalViewer = NULL;
    }
    
    if (picker)
    {
        picker->Delete();
        picker = NULL;
    }

    loaded = false;
    transSlider->setValue(0);  // Must be after loaded is    currDir = QFileInfo(fileName).path(); set to false.
    coronalSlider->setValue(0);
    sagittalSlider->setValue(0);
    
    if (setCrossHairAction->isChecked())
    {
        transXHair->AxesOff();
        coronalXHair->AxesOff();
        sagittalXHair->AxesOff();
        setCrossHairAction->setChecked(false);
    }
}


bool PVolumeViewer::loadFromDir(const QString &dirName)
{
    uninstallPipeline();  // Reset
    vtkDICOMImageReader *rd = vtkDICOMImageReader::New();
    rd->SetDirectoryName(dirName.toAscii().data());
    rd->Update();
    input = rd->GetOutputPort();
    
    long errcode = rd->GetErrorCode();
    if (errcode != 0)
    {
        QMessageBox::critical(this, appName,
            QString("Directory %1 does not contain DICOM image").
            arg(dirName));
        return false;
    }
    
    reader = rd;
    sourceName = dirName;
    setWindowTitle(QString("%1 - ").arg(appName) + sourceName);
    setupWidgets();
    return true;
}


bool PVolumeViewer::loadFromFile(const QString &fileName)
{
    uninstallPipeline();  // Reset
    vtkMetaImageReader *rd = vtkMetaImageReader::New();
    rd->SetFileName(fileName.toAscii().data());
    rd->Update();
    input = rd->GetOutputPort();
    
    long errcode = rd->GetErrorCode();
    if (errcode != 0)
    {
        QMessageBox::critical(this, appName,
            QString("File %1 does not contain a supported volume image").
            arg(fileName));
        return false;
    }

    reader = rd;
    sourceName = fileName;
    setWindowTitle(QString("%1 - ").arg(appName) + sourceName);
    setupWidgets();
    return true;
}


void PVolumeViewer::setupWidgets()
{
    // Get volume size
    vtkImageAlgorithm *algo;
    algo = vtkImageAlgorithm::SafeDownCast(input->GetProducer());
    if (!algo)
    {
        cout << "Error: In PVolumeViewer::setupWidgets(): "
            << "Input is not from vtkImageAlgorithm.\n" << flush;
        return;
    }

    int *ip = algo->GetOutput()->GetExtent();
    
    if (ip[0] < 0 || ip[2] < 0 || ip[4] < 0)
        cout << "Warning: In PVolumeViewer::setupWidgets(): " <<
            "Lower bound of extent is negative.\n" << flush;
    
    imageWidth = ip[1] + 1;
    imageHeight = ip[3] + 1;
    imageDepth = ip[5] + 1;
    
    if (imageWidth == 0 || imageHeight == 0 || imageDepth == 0)
    {
        cout << "Error: In PVolumeViewer::setupWidgets(): "
            "Image is not 3D.\n" <<  flush;
        return;
    }
    
    transSlider->setRange(0, imageDepth - 1);
    coronalSlider->setRange(0, imageHeight - 1);
    sagittalSlider->setRange(0, imageWidth - 1);

    installPipeline();
    resetCameras();
    
    // Set to slice in middle of volume and reset window level
    transSlider->setValue(imageDepth / 2);
    coronalSlider->setValue(imageHeight / 2);
    sagittalSlider->setValue(imageWidth / 2);
    resetWindowLevel();
}


void PVolumeViewer::resetCameras()
{   
    // Reset transverse viewer's camera
    vtkRenderer *renderer = transViewer->GetRenderer();
    vtkCamera *camera = renderer->GetActiveCamera();
    renderer->ResetCamera();

    // Reset coronal viewer's camera
    renderer = coronalViewer->GetRenderer();
    camera = renderer->GetActiveCamera();
    camera->SetViewUp(0.0, 0.0, -1.0);
    renderer->ResetCamera();
    
    // Flip coronal view
    double posx, posy, posz;
    double fpx, fpy, fpz;
    double dx, dy, dz;
    camera->GetPosition(posx, posy, posz);
    camera->GetFocalPoint(fpx, fpy, fpz); 
    dx = posx - fpx;
    dy = posy - fpy;
    dz = posz - fpz;
    posx = fpx - dx;
    posy = fpz - dy;
    posz = fpz - dz;
    camera->SetPosition(posx, posy, posz);

    // Reset sagittal viewer's camera
    renderer = sagittalViewer->GetRenderer();
    camera = renderer->GetActiveCamera();
    camera->SetViewUp(0.0, 0.0, -1.0);
    renderer->ResetCamera();
}


bool PVolumeViewer::saveToFile(const QString &fileName)
{
    vtkMetaImageWriter *writer = vtkMetaImageWriter::New();
    writer->SetFileName(fileName.toAscii().data());
    writer->SetCompression(true);
    writer->SetInputConnection(input);
    writer->Write();
    writer->Delete();
    return true;
}


bool PVolumeViewer::saveView(const QString &fileName, int type)
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


void PVolumeViewer::setWindowLevel(int window, int level)
{
    transViewer->SetColorLevel(level);
    transViewer->SetColorWindow(window);
    transViewer->Render();
    coronalViewer->SetColorLevel(level);
    coronalViewer->SetColorWindow(window);
    coronalViewer->Render();
    sagittalViewer->SetColorLevel(level);
    sagittalViewer->SetColorWindow(window);
    sagittalViewer->Render();
    QString msg = QString("(%1, %2, %3) (L:%4, W:%5)").
        arg(sagittalSlice).arg(coronalSlice).arg(transSlice).
        arg(level).arg(window);
    statusBar()->showMessage(msg);
}


void PVolumeViewer::setTransCrossHair()
{
    vtkImageAlgorithm *algo =
        vtkImageAlgorithm::SafeDownCast(input->GetProducer());
    double *spacing = algo->GetOutput()->GetSpacing();
    vtkCamera *camera;
    double *focal, *pos, top;
    double p[3];
    
    // XY plane
    camera = transViewer->GetRenderer()->GetActiveCamera();
    focal = camera->GetFocalPoint();
    pos = camera->GetPosition();
    top = (focal[2] + pos[2]) / 2;
    p[0] = sagittalSlice * spacing[0];
    p[1] = coronalSlice * spacing[1];
    p[2] = top;        
    transXHair->SetFocalPoint(p);
}


void PVolumeViewer::setCoronalCrossHair()
{
    vtkImageAlgorithm *algo =
        vtkImageAlgorithm::SafeDownCast(input->GetProducer());
    double *spacing = algo->GetOutput()->GetSpacing();
    vtkCamera *camera;
    double *focal, *pos, top;
    double p[3];
    
    // XZ plane
    camera = coronalViewer->GetRenderer()->GetActiveCamera();
    focal = camera->GetFocalPoint();
    pos = camera->GetPosition();
    top = (focal[1] + pos[1]) / 2;
    p[0] = sagittalSlice * spacing[0];
    p[1] = top;
    p[2] = transSlice * spacing[2];
    coronalXHair->SetFocalPoint(p);
}


void PVolumeViewer::setSagittalCrossHair()
{
    vtkImageAlgorithm *algo =
        vtkImageAlgorithm::SafeDownCast(input->GetProducer());
    double *spacing = algo->GetOutput()->GetSpacing();
    vtkCamera *camera;
    double *focal, *pos, top;
    double p[3];
    
    // YZ plane
    camera = sagittalViewer->GetRenderer()->GetActiveCamera();
    focal = camera->GetFocalPoint();
    pos = camera->GetPosition();
    top = (focal[0] + pos[0]) / 2;
    p[0] = top;
    p[1] = coronalSlice * spacing[1];
    p[2] = transSlice * spacing[2];
    sagittalXHair->SetFocalPoint(p);
}


void PVolumeViewer::updateCrossHairs(vtkImageViewer2 *viewer, double *pos)
{
    vtkImageAlgorithm *algo =
        vtkImageAlgorithm::SafeDownCast(input->GetProducer());
    double *spacing = algo->GetOutput()->GetSpacing();
    
    if (viewer == transViewer)
    {
        coronalSlider->setValue((int) pos[1] / spacing[1]);
        sagittalSlider->setValue((int) pos[0] / spacing[0]);
    }
    else if (viewer == coronalViewer)
    {
        transSlider->setValue((int) pos[2] / spacing[2]);
        sagittalSlider->setValue((int) pos[0] / spacing[0]);
    }
    else if (viewer == sagittalViewer)
    {
        transSlider->setValue((int) pos[2] / spacing[2]);
        coronalSlider->setValue((int) pos[1] / spacing[1]);
    }
}


//----- PVolumeViewerCallback functions ----------

PVolumeViewerCallback::PVolumeViewerCallback()
{
    pointData = vtkPointData::New();
}


PVolumeViewerCallback::~PVolumeViewerCallback()
{
    pointData->Delete();
}


void PVolumeViewerCallback::Execute(vtkObject *caller,
    unsigned long eventId, void *callData)
{
    vtkRenderWindowInteractor *interactor = viewer->
        GetRenderWindow()->GetInteractor();
    vtkInteractorStyle *style = vtkInteractorStyle::SafeDownCast(
        interactor->GetInteractorStyle());
        
    if (!dview->setCrossHairAction->isChecked())
    {
        if (eventId == vtkCommand::LeftButtonPressEvent)
        {
            style->OnLeftButtonDown();
            return;
        }
        else if (eventId == vtkCommand::LeftButtonReleaseEvent)
        {
            style->OnLeftButtonUp();
            return;
        }
    }
    else
    {
        if (eventId == vtkCommand::LeftButtonPressEvent)
            dview->crossHairOn = true;
        else if (eventId == vtkCommand::LeftButtonReleaseEvent)
            dview->crossHairOn = false;
    }
            
    if (!dview->loaded)
        return;
        
    vtkImageData *data = viewer->GetInput();
    vtkImageActor *actor = viewer->GetImageActor();

    // Pick at the mouse location provided by the interactor
    vtkPropPicker *picker = dview->picker;
    vtkRenderer *renderer = viewer->GetRenderer();
    picker->Pick(interactor->GetEventPosition()[0],
        interactor->GetEventPosition()[1], 0.0, renderer);

    // There could be other props assigned to this picker, so 
    // make sure we picked the image actor
    vtkAssemblyPath* path = picker->GetPath();
    bool validPick = false;

    if (path)
    {
        vtkCollectionSimpleIterator sit;
        path->InitTraversal(sit);
        vtkAssemblyNode *node;
    
        for (int i = 0; i < path->GetNumberOfItems() && !validPick; ++i)
        {
            node = path->GetNextNode(sit);
            if (actor == vtkImageActor::SafeDownCast(node->GetViewProp()))
                validPick = true;
        }
    }

    if (!validPick)
    {
        dview->statusBar()->showMessage(QString());
    
        // Pass the event further on
        if (eventId == vtkCommand::LeftButtonPressEvent)
            style->OnLeftButtonDown();
        else if (eventId == vtkCommand::MouseMoveEvent)
            style->OnMouseMove();
        else if (eventId == vtkCommand::LeftButtonReleaseEvent)
            style->OnLeftButtonUp();

        return;
    }

    // Get the world coordinates of the pick
    double pos[3];
    picker->GetPickPosition(pos);

    // Fixes some numerical problems with the picking
    double *bounds = actor->GetDisplayBounds();
    int axis = viewer->GetSliceOrientation();
    pos[axis] = bounds[2*axis];

    vtkPointData* pd = data->GetPointData();
    if (!pd)
        return;
        
    pointData->InterpolateAllocate(pd, 1, 1);

    // Use tolerance as a function of size of source data
    double tol2 = data->GetLength();
    tol2 = tol2 ? tol2*tol2 / 1000.0 : 0.001;

    // Find the cell that contains pos
    int subId;
    double pcoords[3], weights[8];
    vtkCell* cell = data->FindAndGetCell(pos, NULL, -1, tol2, subId, 
        pcoords, weights);
    
    if (cell)
    {
        if (dview->setCrossHairAction->isChecked() &&
            dview->crossHairOn)
        {
            dview->updateCrossHairs(viewer, pos);
            return;
        }
        
        // Interpolate the point data
        pointData->InterpolatePoint(pd, 0, cell->PointIds, weights);
        // int components = pointData->GetScalars()->GetNumberOfComponents();
        double* tuple = pointData->GetScalars()->GetTuple(0);

        QString msg =
            QString("(x:%1, y:%2, z:%3) HU:%4; Slice:(%5, %6, %7); ").
            arg((int) pos[0]).arg((int) pos[1]).arg((int) pos[2]).
            arg(tuple[0]).arg(dview->sagittalSlice).
            arg(dview->coronalSlice).arg(dview->transSlice);
    
        int level = viewer->GetColorLevel();
        int window = viewer->GetColorWindow();
        msg += QString("(L:%1, W:%2)").arg(level).arg(window);

        if (viewer != dview->transViewer)
        {
            dview->transViewer->SetColorLevel(level);
            dview->transViewer->SetColorWindow(window);
            dview->transViewer->Render();
        }
        if (viewer != dview->coronalViewer)
        {
            dview->coronalViewer->SetColorLevel(level);
            dview->coronalViewer->SetColorWindow(window);
            dview->coronalViewer->Render();
        }
        if (viewer != dview->sagittalViewer)
        {
            dview->sagittalViewer->SetColorLevel(level);
            dview->sagittalViewer->SetColorWindow(window);
            dview->sagittalViewer->Render();
        }
    
        dview->statusBar()->showMessage(msg);
    
        // Pass the event further on
        style->OnMouseMove();
    }
}
