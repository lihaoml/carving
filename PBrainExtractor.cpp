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
    vtkImageData *data = reader->GetOutput();
    dijkstra2D (data, 164, 172, 201, 205, 105);
}