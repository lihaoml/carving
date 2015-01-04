/* PBrainExtractor.h

   Brain extraction tool.

   Copyright 2103, 2014 National University of Singapore
   Author: Leow Wee Kheng
*/

#ifndef PBRAINEXTRACTOR_H
#define PBRAINEXTRACTOR_H

#include "PVolumeSegmenter.h"


class PBrainExtractor: public PVolumeSegmenter
{
    Q_OBJECT
    
public:
    PBrainExtractor();
    ~PBrainExtractor();
       
protected slots:
    void showBrainExtractionDialog();

protected:
    void addWidgets();
    void addActions();
    void addMenus();
    void addToolBars();
    
    QAction *extractBrainAction;
    
    QMenu *brainMenu;
    
    QToolBar *brainToolBar;
    
    // Internal variables

    // Supporting functions

};

#endif 