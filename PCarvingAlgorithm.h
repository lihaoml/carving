//
//  PCarvingAlgorithm.h
//  
//
//  Created by Li Hao on 4/1/15.
//
//

#ifndef ____PCarvingAlgorithm__
#define ____PCarvingAlgorithm__

#include "vtkImageData.h"

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


void dijkstra2D ( vtkImageData *data, int _x1, int _y1, int _x2, int _y2, int _z );

#endif /* defined(____PCarvingAlgorithm__) */
