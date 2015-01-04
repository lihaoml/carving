/* PVoiWidget.h

   Volume of interest widget.

   Copyright 2103, National University of Singapore
   Author: Leow Wee Kheng
*/

#ifndef PVOIWIDGET_H
#define PVOIWIDGET_H

#include <QObject>
#include "vtkLineWidget.h"
#include "vtkImageActor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"


class PVoiWidget: public QObject
{
    Q_OBJECT
    
    friend class PVoiWidgetCallback;

public:
    PVoiWidget();
    ~PVoiWidget();
    enum PlaneType {XY, YZ, XZ};
    enum LineType {Top, Bottom, Left, Right};
    void alignXYPlane();
    void alignYZPlane();
    void alignXZPlane();
    void setColor(LineType type, int red, int green, int blue);
    void initPosition(vtkImageActor *actor, vtkCamera *camera, int margin);
    void setInteractor(vtkRenderWindowInteractor *interactor);
    double *getBounds();

signals:
    void topLineChanged(double pos);
    void bottomLineChanged(double pos);
    void leftLineChanged(double pos);
    void rightLineChanged(double pos);
    
public slots:
    void show(bool option);
    void show();
    void hide();
    void setTopLine(double pos);
    void setBottomLine(double pos);
    void setLeftLine(double pos);
    void setRightLine(double pos);
    void update();
    
protected:
    PlaneType plane; 
    vtkLineWidget *topLine;
    vtkLineWidget *botLine;
    vtkLineWidget *leftLine;
    vtkLineWidget *rightLine;
    vtkRenderWindowInteractor *interactor;
    
    double topXY;  // Top variables, keep lines in between image and camera.
    double topYZ;
    double topXZ;
};

#endif