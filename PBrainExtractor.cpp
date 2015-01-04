/* PBrainExtractor.cpp

  Brain extraction tool.

   Copyright 2012, 2103, 2014 National University of Singapore
   Author: Leow Wee Kheng
*/

#include "PBrainExtractor.h"
#include "vtkImageData.h"
#include <QtGui>


#include "PCarvingAlgorithm.h"

PBrainExtractor::PBrainExtractor()
{
    appName = QString("Brain Extractor");
    setWindowTitle(appName);
    
    // Create GUI
    addWidgets();
    addActions();
    addMenus();
    addToolBars();
}


PBrainExtractor::~PBrainExtractor()
{

}


// Create additional widgets and objects

void PBrainExtractor::addWidgets()
{
}


void PBrainExtractor::addActions()
{    
    extractBrainAction = new QAction(tr("Extract Brain"), this);
    extractBrainAction->setIcon(QIcon(":/images/fullwhite.png"));
    extractBrainAction->setShortcut(tr("Ctrl+B"));
    extractBrainAction->setStatusTip(tr("Apply brain extraction."));
    extractBrainAction->setCheckable(true);
    connect(extractBrainAction, SIGNAL(triggered()),
        this, SLOT(showBrainExtractionDialog()));
}


void PBrainExtractor::addMenus()
{    
    brainMenu = new QMenu(tr("Brain"));
    menuBar()->insertMenu(outputMenu->menuAction(), brainMenu);
    brainMenu->addAction(extractBrainAction);
}


void PBrainExtractor::addToolBars()
{       
    addToolBarBreak();
    brainToolBar = addToolBar(tr("Brain"));
    brainToolBar->addAction(extractBrainAction);
}


//----- Slot functions ----------
void PBrainExtractor::showBrainExtractionDialog()
{
    // vtkImageData *data = vtkImageData::New();
    // data->ShallowCopy(reader->GetOutput());
    int x1 = 164, y1 = 172, z1 = 105;
    // int x2 = 201, y2 = 205, z2 = z1;
    int x2 = 194, y2 = 244, z2 = z1;
    int x3 = 168, y3 = 190, z3 = 88;
    int x4 = 181, y4 = 201, z4 = z3;
    vtkImageData *data = reader->GetOutput();
    dijkstra2D (data, x1, y1, x2, y2, z1);
    dijkstra2D (data, x3, y3, x4, y4, z3);
    
    // dijkstra3D (data, x1, y1, z1, x3, y3, z3);
    // dijkstra3D (data, x2, y2, z2, x4, y4, z4);
    std::vector<Pos3D> boundary1;
    std::vector<Pos3D> boundary2;
    
    averageRank3D (data, x1, y1, z1, x3, y3, z3, boundary1);
    averageRank3D (data, x2, y2, z2, x4, y4, z4, boundary2);
    
    for (unsigned i = 0; i < boundary1.size(); i++)
    {
        std::cout << "(" << boundary1[i].x << ", " << boundary1[i].y << ", " << boundary1[i].z << ") --- ";
        std::cout << "(" << boundary2[i].x << ", " << boundary2[i].y << ", " << boundary2[i].z << ")" << std::endl;
    }
  
    for (unsigned i = 0; i < boundary1.size(); i++)
        dijkstra2D ( data, boundary1[i].x, boundary1[i].y
                   , boundary2[i].x, boundary2[i].y, boundary1[i].z);
   
}