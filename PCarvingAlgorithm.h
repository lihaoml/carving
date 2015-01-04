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
#include <vector>



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

struct Pos3D
{
    int x;
    int y;
    int z;
    bool operator==(const Pos3D& a) const
    {
        return (x == a.x && y == a.y && z == a.z);
    }
    // this is necesarily implemented like this for the set container
    bool operator<(const Pos3D& a) const
    {
        return (x < a.x || (x == a.x && y < a.y)
                || (x == a.x && y == a.y && z < a.z)
                );
    }
    // here we define a strict less than
    bool operator<=(const Pos3D& a) const
    {
        return (x <= a.x) && (y <= a.y) && (z <= a.z);
    }
    Pos3D (unsigned _x, unsigned _y, unsigned _z) {x = _x; y = _y; z = _z;}
    Pos3D () {};
    
    
};


template<typename T>
struct Node
{
    double distance;
    T previous;
};


void dijkstra2D ( vtkImageData *data, int _x1, int _y1, int _x2, int _y2, int _z );
void dijkstra3D ( vtkImageData *data, int _x1, int _y1, int _z1, int _x2, int _y2, int _z2 );

void averageRank3D ( vtkImageData *data, int _x1, int _y1, int _z1, int _x2, int _y2, int _z2, std::vector<Pos3D>& result );


#endif /* defined(____PCarvingAlgorithm__) */
