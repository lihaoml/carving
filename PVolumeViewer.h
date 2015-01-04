/* PVolumeViewer.h

   3-plane volume viewer.

   Copyright 2012, 2103, 2014 National University of Singapore
   Author: Leow Wee Kheng
*/


#ifndef PVOLUMEVIEWER_H
#define PVOLUMEVIEWER_H

#include <QMainWindow>

class QAction;
class QComboBox;
class QSlider;

#include "QVTKWidget.h"
#include "vtkImageReader2.h"
#include "vtkImageViewer2.h"
#include "vtkAlgorithmOutput.h"
#include "vtkPropPicker.h"

#include "vtkCursor3D.h"
#include "vtkPolyDataMapper.h"


class PVolumeViewer: public QMainWindow
{
    Q_OBJECT
    
    friend class PVolumeViewerCallback;

public:
    PVolumeViewer();
    ~PVolumeViewer();
    void setAppName(const QString &name);
        
protected:
    void closeEvent(QCloseEvent *event);
    
protected slots:
    void loadDir();
    void loadFile();
    void saveFile();
    void setViewPane();
    void resetZoom();
    void resetWindowLevel();
    void setWindowLevel(int type);
    void saveTransView();
    void saveCoronalView();
    void saveSagittalView();
    void info();
    void help();
    void about();

    void resetCrossHair();
    void setCrossHair(bool on);
    void transViewSetSlice(int slice);
    void coronalViewSetSlice(int slice);
    void sagittalViewSetSlice(int slice);
    void updateViewers();

protected:
    void createWidgets();
    void createCrossHairs();
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();

    QAction *loadDirAction;
    QAction *loadFileAction;
    QAction *saveFileAction;
    QAction *exitAction;
    QAction *setViewPaneAction;
    QAction *setCrossHairAction;
    QAction *resetZoomAction;
    QAction *resetWindowLevelAction;
    QAction *saveTransViewAction;
    QAction *saveCoronalViewAction;
    QAction *saveSagittalViewAction;
    QAction *infoAction;
    QAction *helpAction;
    QAction *aboutAction;
    
    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *setViewModeMenu;
    QMenu *saveViewMenu;
    QMenu *helpMenu;
    
    QToolBar *fileToolBar;
    QToolBar *viewToolBar;
    QToolBar *helpToolBar;
    
    QComboBox *windowLevelBox;
    
    // Input / output
    vtkImageReader2 *reader;
    vtkAlgorithmOutput *input;
    vtkAlgorithmOutput *extInput;
    
    // Widgets and VTK objects for volume viewer (transverse view)
    QVTKWidget *transWidget;
    vtkImageViewer2 *transViewer;
    QSlider *transSlider;
    QWidget *pane00;
    
    // Free (unused pane)
    QWidget *pane01;
    
    // Widgets and VTK objects for volume viewer (coronal view)
    QVTKWidget *coronalWidget;
    vtkImageViewer2 *coronalViewer;
    QSlider *coronalSlider;
    QWidget *pane10;
    
    // Widgets and VTK objects for volume viewer (sagittal view)
    QVTKWidget *sagittalWidget;
    vtkImageViewer2 *sagittalViewer;
    QSlider *sagittalSlider;
    QWidget *pane11;
    
    // Picker
    vtkPropPicker *picker;
    
    // Cross hairs
    vtkCursor3D *transXHair;
    vtkPolyDataMapper *transXMapper;
    vtkActor *transXActor;
    
    vtkCursor3D *coronalXHair;
    vtkPolyDataMapper *coronalXMapper;
    vtkActor *coronalXActor;
    
    vtkCursor3D *sagittalXHair;
    vtkPolyDataMapper *sagittalXMapper;
    vtkActor *sagittalXActor;
    
    // Internal variables.
    QString appName;
    QString sourceName;
    QString readDir;  // Current read directory
    QString writeDir; // Current write directory
    int numPane;
    bool loaded;
    int winWidth, winHeight;
    int imageWidth, imageHeight, imageDepth;
    int transSlice, coronalSlice, sagittalSlice;
    bool crossHairOn;
    
    // Supporting methods
    void installPipeline();
    void uninstallPipeline();
    bool loadFromDir(const QString &dirName);
    bool loadFromFile(const QString &fileName);
    void setupWidgets();
    void resetCameras();
    bool saveToFile(const QString &fileName);
    
    void saveView(int type);
    bool saveView(const QString &fileName, int type);
    void setWindowLevel(int window, int level);
    
    void setTransCrossHair();
    void setCoronalCrossHair();
    void setSagittalCrossHair();
    void updateCrossHairs(vtkImageViewer2 *viewer, double *pos);
};

#endif