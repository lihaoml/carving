//
//  PCarvingAlgorithm.cpp
//  
//
//  Created by Li Hao on 4/1/15.
//
//

#include "PCarvingAlgorithm.h"
#include <set>
#include <vector>

std::ostream& operator<<(std::ostream& os, const Pos3D& obj)
{
    os << "(" << obj.x << ", " << obj.y << ", " << obj.z << ")";
    return os;
}

// input: voxcel location in index
// output:
void dijkstra2D ( vtkImageData *data, int _x1, int _y1, int _x2, int _y2, int _z )
{
    int dims [3];
    data->GetDimensions(dims);
    const int nComp = data->GetNumberOfScalarComponents();
    std::cout << "Dimensions: " << dims[0] << ", " << dims[1] << ", " << dims[2] << std::endl;
    
    // std::cout << "Components: " << nComp << std::endl;
    // std::cout << "Scalar Type: " << data->GetScalarTypeAsString() << std::endl;
    
    short* vxl = static_cast<short*>(data->GetScalarPointer());
    
    double spacing[3];
    data->GetSpacing(spacing);
    
    std::cout << "Spacing: " << spacing[0] << ", " << spacing[1] << ", " << spacing[2] << std::endl;
    
    int x1 = static_cast<int> (_x1 / spacing[0]);
    int y1 = static_cast<int> (_y1 / spacing[1]);
    int z = static_cast<int> (_z / spacing[2]);
    int x2 = static_cast<int> (_x2 / spacing[0]);
    int y2 = static_cast<int> (_y2 / spacing[1]);
    
    int idx1 = z * (dims[0]*dims[1]) + y1 * dims[0] + x1;
    // std::cout << "(" << x1 << ", " << y1 << ", " << z << ") = " << vxl[idx1] << std::endl;
    
    int idx2 = z * (dims[0]*dims[1]) + y2 * dims[0] + x2;
    // std::cout << "(" << x2 << ", " << y2 << ", " << z << ") = " << vxl[idx2] << std::endl;
    
    // the small rectangular region bounded by (x1, y1) and (x2, y2)
    unsigned width = std::abs(x2 - x1) + 1;
    unsigned height = std::abs(y2 - y1) + 1;
    // std::cout << "width = " << width << std::endl;
    // std::cout << "height = " << height << std::endl;
    
    // prepare the gradient map
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
    
    
    // print out the gradient map show the gradient map, for debug use only
    for (int j = height-1; j >= 0; j--)
    {
        for (unsigned i = 0; i < width; i++ )
        {
            gradient[i][j] = maxG - gradient[i][j];
            // std::cout << std::setw(4) << gradient[i][j];
        }
        // std::cout << std::endl;
    }
    
    // initialize the nodes of the graph, prepare for the dijkstra algorithm
    std::vector< std::vector<Node<Pos> > > nodes;
    for ( unsigned i = 0; i < width; i++ )
    {
        Node<Pos> initNode;
        initNode.distance = 1000000;
        initNode.previous.x = -1;
        initNode.previous.y = -1;
        std::vector<Node<Pos> > c (height, initNode);
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
    
    
/*    std::cout << "========distance map============" << std::endl;
    // show the distance map
    for (int j = height-1; j >= 0; j--)
    {
        for (unsigned i = 0; i < width; i++ )
            std::cout << std::setw(4) << nodes[i][j].distance;
        std::cout << std::endl;
    }
    std::cout << "========path====================" << std::endl;
  */
    // show the path
    std::vector< std::vector<int> > path;
    for ( unsigned i = 0; i < width; i++ )
    {
        std::vector<int> p (height, 0);
        path.push_back(p);
    }
    
    Node<Pos> backward = nodes[width-1][height-1];
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
        //  std::cout << std::setw(4) << path[i][j];
            if (path[i][j] == 1)
            {
                // draw the seam
                vxl[(z+1) * (dims[0]*dims[1]) + (y1+j) * dims[0] + x1 + i] = 1000;
                vxl[z * (dims[0]*dims[1]) + (y1+j) * dims[0] + x1 + i] = 1000;
            }
        }
        std::cout << std::endl;
    }
}


void dijkstra3D ( vtkImageData *data, int _x1, int _y1, int _z1, int _x2, int _y2, int _z2 )
{

    int dims [3];
    data->GetDimensions(dims);
    short* vxl = static_cast<short*>(data->GetScalarPointer());
    
    double spacing[3];
    data->GetSpacing(spacing);
    int x1 = static_cast<int> (_x1 / spacing[0]);
    int y1 = static_cast<int> (_y1 / spacing[1]);
    int z1 = static_cast<int> (_z1 / spacing[2]);
    int x2 = static_cast<int> (_x2 / spacing[0]);
    int y2 = static_cast<int> (_y2 / spacing[1]);
    int z2 = static_cast<int> (_z2 / spacing[2]);
    
    
    int stepZ = z1 < z2 ? 1 : -1;
    int stepX = x1 < x2 ? 1 : -1;
    int stepY = y1 < y2 ? 1 : -1;
    
    // the small cube bounded by (x1, y1, z1) and (x2, y2, z2)
    unsigned width = std::abs(x2 - x1) + 1;
    unsigned height = std::abs(y2 - y1) + 1;
    unsigned depth = std::abs(z2 - z1) + 1;
    
    // instead of using the gradient map in 2D,
    // let's try the intensity map here
    std::vector< std::vector< std::vector<short> > > energy;
    // the nodes construct a 3D graph now
    std::vector< std::vector< std::vector<Node<Pos3D> > > > nodes;
    for ( unsigned i = 0; i < width; i++ )
    {
        std::vector< std::vector<Node<Pos3D> > > nodesInternal;
        std::vector< std::vector<short> > energyInternal;
        for (unsigned j = 0; j < height; j++)
        {
            Node<Pos3D> initNode;
            initNode.distance = 1000000;
            initNode.previous.x = -1;
            initNode.previous.y = -1;
            initNode.previous.z = -1;
            std::vector<Node<Pos3D> > c (depth, initNode);
            nodesInternal.push_back(c);
            
            std::vector<short> e(depth);
            for (unsigned k = 0; k < depth; k++)
                e[k] = 1000 - vxl[(z1+k*stepZ)*dims[0]*dims[1] + (y1+j*stepY)*dims[0] + (x1+i)*stepX];
            energyInternal.push_back(e);
        }
        nodes.push_back(nodesInternal);
        energy.push_back(energyInternal);
    }
    
    nodes[0][0][0].distance = 0;
    std::set<Pos3D> queue;
    Pos3D start(0, 0, 0);
    
    
    queue.insert(start);
    // in the 3D search, we impose the constrain that the graph is one directional along z axis
    // meaning that the edges are directed only along z1 -> z2
    // also, we want one voxel per z, so no planar edge is added
    
    int stepW = 2; // when stepping z, we allow a 5x5 window to be feasible. this can be extended to be flexible based on (x1, y1, z1) and (x2, y2, z2);
    Pos3D pL (0, 0, 0), pU(width-1, height-1, depth-1); // lower and upper bound
    
    while (!queue.empty())
    {
        Pos3D pos = *queue.begin();
        queue.erase(queue.begin());
        double d = nodes[pos.x][pos.y][pos.z].distance;
        for (int i = -stepW; i <= stepW; i++)
        {
            for (int j = -stepW; j <= stepW; j++)
            {
                Pos3D nextPos;
                nextPos.x = pos.x + i;
                nextPos.y = pos.y + j;
                nextPos.z = pos.z + 1;
                
                if (nextPos <= pU && pL <= nextPos)
                {
                    // consider this candidate
                    double newDistance = d + energy[pos.x][pos.y][pos.z];
                    if (newDistance < nodes[nextPos.x][nextPos.y][nextPos.z].distance)
                    {
                        nodes[nextPos.x][nextPos.y][nextPos.z].distance = newDistance;
                        nodes[nextPos.x][nextPos.y][nextPos.z].previous = pos;
                        queue.insert(nextPos);
                    }
                }
            }
        }
        
    };
    
    // show the path
    std::vector< std::vector< std::vector<int> > > path;
    for ( unsigned i = 0; i < width; i++ )
    {
        std::vector< std::vector<int> > pathInternal;
        for (unsigned j = 0; j < height; j++)
        {
            std::vector<int> p (depth, 0);
            pathInternal.push_back(p);
        }
        path.push_back(pathInternal);
    }
    
    Node<Pos3D> backward = nodes[width-1][height-1][depth-1];
    path[width-1][height-1][depth-1] = 1;
    
    while (!(backward.previous == start))
    {
        // std::cout << backward.previous << std::endl;
        path[backward.previous.x][backward.previous.y][backward.previous.z] = 1;
        backward = nodes[backward.previous.x][backward.previous.y][backward.previous.z];
    };
    path[start.x][start.y][start.z] = 1;

    for (int j = height-1; j >= 0; j--)
        for (unsigned i = 0; i < width; i++ )
            for (unsigned k = 0; k < depth; k++) {
                if (path[i][j][k] == 1)
                {
                    // draw the seam
                    vxl[(z1 + k*stepZ) * (dims[0]*dims[1]) + (y1+j*stepY) * dims[0] + x1 + i*stepX] = 1000;
                    vxl[((z1+1) + k*stepZ) * (dims[0]*dims[1]) + (y1+j*stepY) * dims[0] + x1 + i*stepX] = 1000;
                }
            }
    
}

void averageRank3D ( vtkImageData *data, int _x1, int _y1, int _z1, int _x2, int _y2, int _z2
                    , std::vector<Pos3D>& result)
{
    
    int dims [3];
    data->GetDimensions(dims);
    short* vxl = static_cast<short*>(data->GetScalarPointer());
    
    double spacing[3];
    data->GetSpacing(spacing);
    int x1 = static_cast<int> (_x1 / spacing[0]);
    int y1 = static_cast<int> (_y1 / spacing[1]);
    int z1 = static_cast<int> (_z1 / spacing[2]);
    int x2 = static_cast<int> (_x2 / spacing[0]);
    int y2 = static_cast<int> (_y2 / spacing[1]);
    int z2 = static_cast<int> (_z2 / spacing[2]);
    
    
    int stepZ = z1 < z2 ? 1 : -1;
    int stepX = x1 < x2 ? 1 : -1;
    int stepY = y1 < y2 ? 1 : -1;
    
    // the small cube bounded by (x1, y1, z1) and (x2, y2, z2)
    unsigned width = std::abs(x2 - x1) + 1;
    unsigned height = std::abs(y2 - y1) + 1;
    unsigned depth = std::abs(z2 - z1) + 1;
    
    // instead of using the gradient map in 2D,
    // let's try the intensity map here
    std::vector< std::vector< std::vector<short> > > energy;
    for ( unsigned i = 0; i < width; i++ )
    {
        std::vector< std::vector<short> > energyInternal;
        for (unsigned j = 0; j < height; j++)
        {
            std::vector<short> e(depth);
            for (unsigned k = 0; k < depth; k++)
                e[k] = 1000 - vxl[(z1+k*stepZ)*dims[0]*dims[1] + (y1+j*stepY)*dims[0] + (x1+i)*stepX];
            energyInternal.push_back(e);
        }
        energy.push_back(energyInternal);
    }
    std::vector< Pos > path (depth);
    for ( unsigned k = 1; k < depth-1; k++)
    {
        int d = width * height;
        std::vector<int> rank_e(d);
        std::vector<int> rank_d(d);
        
        double center_x = (double)(width) * (double)k / (depth-1.0);
        double center_y = (double)(height) * (double)k / (depth-1.0);
        
        int minAvgRank = d * 2;
        int minAvgRankIdx = -1;
        for (int i = 0; i < d; i++) {
            int x = i / height;
            int y = i % height;
            
            // get the rank for energy
            short e = energy[x][y][k];
            int rank = 0;
            for (int j = 0; j < d; j++) {
                int x = j / height;
                int y = j % height;
                if (e > energy[x][y][k])
                    rank++;
            }
            rank_e[i] = rank;
            
            // get the rank for distance
            rank = 0;
            double dis = (x - center_x)*(x - center_x) + (y - center_y)*(y - center_y);
            for (int j = 0; j < d; j++) {
                int x = j / height;
                int y = j % height;
                double dis2 = (x - center_x)*(x - center_x) + (y - center_y)*(y - center_y);
                if (dis > dis2)
                    rank++;
            }
            rank_d[i] = rank;
            
            if (rank_d[i] + rank_e[i] < minAvgRank)
            {
                minAvgRank = rank_d[i] + rank_e[i];
                minAvgRankIdx = i;
            }
        }
        path[k].x = minAvgRankIdx / height;
        path[k].y = minAvgRankIdx % height;
        
        // show the path
        vxl[(z1 + k*stepZ) * (dims[0]*dims[1]) + (y1+path[k].y*stepY) * dims[0] + x1 + path[k].x*stepX] = 1000;
        
        result.push_back(Pos3D( spacing[0] * (x1 + stepX * path[k].x)
                              , spacing[1] * (y1 + stepY * path[k].y)
                              , spacing[2] * (z1 + stepZ * k) ) );
    }
}