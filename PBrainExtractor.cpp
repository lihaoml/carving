/* PBrainExtractor.cpp

  Brain extraction tool.

   Copyright 2012, 2103, 2014 National University of Singapore
   Author: Leow Wee Kheng
*/

#include "PBrainExtractor.h"
#include "vtkImageData.h"
#include <QtGui>
#include <set>


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
// calculate the minimum distance map
struct Pos
{
    unsigned int x;
    unsigned int y;
    bool operator==(const Pos& a) const
    {
        return (x == a.x && y == a.y);
    }
    bool operator<(const Pos& a) const
    {
        return (x < a.x || (x == a.x && y < a.y));
    }
    Pos (unsigned _x, unsigned _y) {x = _x; y = _y;}
    Pos () {};
};

struct Node
{
    double distance;
    Pos previous;
};

void PBrainExtractor::showBrainExtractionDialog()
{
    // vtkImageData *data = vtkImageData::New();
    // data->ShallowCopy(reader->GetOutput());
    vtkImageData *data = reader->GetOutput();
    int dims [3];
    data->GetDimensions(dims);
    const int nComp = data->GetNumberOfScalarComponents();
    std::cout << "Dimensions: " << dims[0] << ", " << dims[1] << ", " << dims[2] << std::endl;
    std::cout << "Components: " << nComp << std::endl;
    std::cout << "Scalar Type: " << data->GetScalarTypeAsString() << std::endl;
    
    short* vxl = static_cast<short*>(data->GetScalarPointer());
    
    double spacing[3];
    data->GetSpacing(spacing);
    int x1 = static_cast<int> (164 / spacing[0]);
    int y1 = static_cast<int> (172 / spacing[1]);
    int z = static_cast<int> (105 / spacing[2]);
    int x2 = static_cast<int> (201 / spacing[0]);
    int y2 = static_cast<int> (205 / spacing[1]);
    
    int idx1 = z * (dims[0]*dims[1]) + y1 * dims[0] + x1;
    std::cout << "(" << x1 << ", " << y1 << ", " << z << ") = " << vxl[idx1] << std::endl;
    
    int idx2 = z * (dims[0]*dims[1]) + y2 * dims[0] + x2;
    std::cout << "(" << x2 << ", " << y2 << ", " << z << ") = " << vxl[idx2] << std::endl;
    
    
    unsigned width = x2 - x1 + 1;
    unsigned height = y2 - y1 + 1;
    std::cout << "width = " << width << std::endl;
    std::cout << "height = " << height << std::endl;
    
    std::vector< std::vector<short> > gradient;
    short maxG = 0;
    for (unsigned i = 0; i < width; i++)
    {
        std::vector<short> g(height);
        for (unsigned j = 0; j < height; j++)
        {
            short huL = vxl[z * (dims[0]*dims[1]) + (y1+j) * dims[0] + x1 + i-1];
            short huR = vxl[z * (dims[0]*dims[1]) + (y1+j) * dims[0] + x1 + i+1];
            short huU = vxl[z * (dims[0]*dims[1]) + (y1+j+1) * dims[0] + x1 + i];
            short huD = vxl[z * (dims[0]*dims[1]) + (y1+j-1) * dims[0] + x1 + i];
            g[j] = static_cast<short>(sqrt(((huL - huR)/2)*((huL - huR)/2) + ((huU - huD)/2)*((huU - huD)/2)));
            if (g[j] > maxG)
                maxG = g[j];
        }
        gradient.push_back(g);
    }
    // show the gradient map
    for (int j = height-1; j >= 0; j--)
    {
        for (unsigned i = 0; i < width; i++ )
        {
            gradient[i][j] = maxG - gradient[i][j];
            std::cout << std::setw(4) << gradient[i][j];
        }
        std::cout << std::endl;
    }
    
    
    std::vector< std::vector<Node> > nodes;
    for ( unsigned i = 0; i < width; i++ )
    {
        Node initNode;
        initNode.distance = 1000000;
        initNode.previous.x = -1;
        initNode.previous.y = -1;
        std::vector<Node> c (height, initNode);
        nodes.push_back(c);
    }
    
    nodes[0][0].distance = 0;
    
    std::set<Pos> queue;
    Pos start(0, 0);
    
    
    queue.insert(start);
    
    while (!queue.empty())
    {
        Pos pos = *queue.begin();
        queue.erase(queue.begin());
        double d = nodes[pos.x][pos.y].distance;
        
        // up
        if ( pos.y + 1 < height &&
            nodes[pos.x][pos.y+1].distance > d + gradient[pos.x][pos.y+1])
        {
            Pos nextPos;
            nextPos.x = pos.x;
            nextPos.y = pos.y+1;
            nodes[nextPos.x][nextPos.y].distance = d+gradient[pos.x][pos.y+1];
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
        /*
        // up left
        if ( pos.y + 1 < height && pos.x > 0 &&
            nodes[pos.x-1][pos.y+1].distance > d + gradient[pos.x-1][pos.y+1] * 2)
        {
            Pos nextPos;
            nextPos.x = pos.x-1;
            nextPos.y = pos.y+1;
            nodes[nextPos.x][nextPos.y].distance = d+gradient[pos.x-1][pos.y+1] * 2;
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
        
        // up right
        if ( pos.y + 1 < height && pos.x + 1 < width &&
            nodes[pos.x+1][pos.y+1].distance > d + gradient[pos.x+1][pos.y+1] * 2)
        {
            Pos nextPos;
            nextPos.x = pos.x+1;
            nextPos.y = pos.y+1;
            nodes[nextPos.x][nextPos.y].distance = d+gradient[pos.x+1][pos.y+1] * 2;
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
        */
        // left
        if ( pos.x > 0 &&
            nodes[pos.x-1][pos.y].distance > d + gradient[pos.x-1][pos.y])
        {
            Pos nextPos;
            nextPos.x = pos.x-1;
            nextPos.y = pos.y;
            nodes[nextPos.x][nextPos.y].distance = d + gradient[pos.x-1][pos.y];
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
        
        // right
        if ( pos.x + 1 < width &&
            nodes[pos.x+1][pos.y].distance > d + gradient[pos.x+1][pos.y])
        {
            Pos nextPos;
            nextPos.x = pos.x+1;
            nextPos.y = pos.y;
            nodes[nextPos.x][nextPos.y].distance = d + gradient[pos.x+1][pos.y];
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
        
        // down
        if ( pos.y > 0 &&
            nodes[pos.x][pos.y-1].distance > d + gradient[pos.x][pos.y-1])
        {
            Pos nextPos;
            nextPos.x = pos.x;
            nextPos.y = pos.y-1;
            nodes[nextPos.x][nextPos.y].distance = d + gradient[pos.x][pos.y-1];
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
        /*
        // down left
        if ( pos.y > 0 && pos.x > 0 &&
            nodes[pos.x-1][pos.y-1].distance > d + gradient[pos.x-1][pos.y-1] * 2)
        {
            Pos nextPos;
            nextPos.x = pos.x-1;
            nextPos.y = pos.y-1;
            nodes[nextPos.x][nextPos.y].distance = d + gradient[pos.x-1][pos.y-1] * 2;
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
        
        // down right
        if ( pos.y > 0 && pos.x + 1 < width &&
            nodes[pos.x+1][pos.y-1].distance > d + gradient[pos.x+1][pos.y-1] * 2)
        {
            Pos nextPos;
            nextPos.x = pos.x+1;
            nextPos.y = pos.y-1;
            nodes[nextPos.x][nextPos.y].distance = d + gradient[pos.x+1][pos.y-1] * 2;
            nodes[nextPos.x][nextPos.y].previous = pos;
            queue.insert(nextPos);
        }
         */ 
    };
    std::cout << "========distance map============" << std::endl;
    // show the distance map
    for (int j = height-1; j >= 0; j--)
    {
        for (unsigned i = 0; i < width; i++ )
            std::cout << std::setw(4) << nodes[i][j].distance;
        std::cout << std::endl;
    }
    std::cout << "========path====================" << std::endl;
    
    // show the path
    std::vector< std::vector<int> > path;
    for ( unsigned i = 0; i < width; i++ )
    {
        std::vector<int> p (height, 0);
        path.push_back(p);
    }
    
    Node backward = nodes[width-1][height-1];
    path[width-1][height-1] = 1;
    while (!(backward.previous == start))
    {
        path[backward.previous.x][backward.previous.y] = 1;
        backward = nodes[backward.previous.x][backward.previous.y];
    };
    path[start.x][start.y] = 1;
    
    for (int j = height-1; j >= 0; j--)
    {
        for (unsigned i = 0; i < width; i++ ) {
            std::cout << std::setw(4) << path[i][j];
            if (path[i][j] == 1)
                // draw the seam
                vxl[z * (dims[0]*dims[1]) + (y1+j) * dims[0] + x1 + i] = 1000;
        }
        std::cout << std::endl;
    }
    
}