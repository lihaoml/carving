/* PVoiWidget.cpp

   Volume of interest widget.

   Copyright 2103, National University of Singapore
   Author: Leow Wee Kheng
*/

#include "PVoiWidget.h"
#include "vtkProperty.h"
#include "vtkCommand.h"

#include <unistd.h>
using namespace std;


// Callback class

class PVoiWidgetCallback: public vtkCommand
{
public:
    static PVoiWidgetCallback *New() { return new PVoiWidgetCallback; }
    void Execute(vtkObject *caller, unsigned long eventId, void *callData);
    PVoiWidget *host;

protected:
    PVoiWidgetCallback();
    ~PVoiWidgetCallback();
};


#define LowBound -2000
#define HighBound 2000


PVoiWidget::PVoiWidget()
{
    // Create line widgets
    topLine = vtkLineWidget::New();
    topLine->ClampToBoundsOn();
    topLine->GetLineProperty()->SetLineWidth(1.0);
    topLine->GetHandleProperty()->SetOpacity(0.0);
    topLine->PlaceWidget(LowBound, HighBound, LowBound, HighBound,
        LowBound, HighBound);
        
    botLine = vtkLineWidget::New();
    botLine->ClampToBoundsOn();
    botLine->GetLineProperty()->SetLineWidth(1.0);
    botLine->GetHandleProperty()->SetOpacity(0.0);
    botLine->PlaceWidget(LowBound, HighBound, LowBound, HighBound,
        LowBound, HighBound);

    leftLine = vtkLineWidget::New();
    leftLine->ClampToBoundsOn();
    leftLine->GetLineProperty()->SetLineWidth(1.0);
    leftLine->GetHandleProperty()->SetOpacity(0.0);
    leftLine->PlaceWidget(LowBound, HighBound, LowBound, HighBound,
        LowBound, HighBound);
        
    rightLine = vtkLineWidget::New();
    rightLine->ClampToBoundsOn();
    rightLine->GetLineProperty()->SetLineWidth(1.0);
    rightLine->GetHandleProperty()->SetOpacity(0.0);
    rightLine->PlaceWidget(LowBound, HighBound, LowBound, HighBound,
        LowBound, HighBound);
    
    // Create callbacks
    PVoiWidgetCallback *callback = PVoiWidgetCallback::New();
    callback->host = this;
    topLine->AddObserver(vtkCommand::InteractionEvent, callback);
    callback->Delete();
    
    callback = PVoiWidgetCallback::New();
    callback->host = this;
    botLine->AddObserver(vtkCommand::InteractionEvent, callback);
    callback->Delete();
    
    callback = PVoiWidgetCallback::New();
    callback->host = this;
    leftLine->AddObserver(vtkCommand::InteractionEvent, callback);
    callback->Delete();
    
    callback = PVoiWidgetCallback::New();
    callback->host = this;
    rightLine->AddObserver(vtkCommand::InteractionEvent, callback);
    callback->Delete();
}


PVoiWidget::~PVoiWidget()
{
    topLine->Delete();
    botLine->Delete();
    leftLine->Delete();
    rightLine->Delete();
}


void PVoiWidget::alignXYPlane()
{
    plane = XY;
    topLine->SetAlignToXAxis();
    botLine->SetAlignToXAxis();
    leftLine->SetAlignToYAxis();
    rightLine->SetAlignToYAxis();
}


void PVoiWidget::alignYZPlane()
{
    plane = YZ;
    topLine->SetAlignToZAxis();
    botLine->SetAlignToZAxis();
    leftLine->SetAlignToYAxis();
    rightLine->SetAlignToYAxis();
}


void PVoiWidget::alignXZPlane()
{
    plane = XZ;
    topLine->SetAlignToZAxis();
    botLine->SetAlignToZAxis();
    leftLine->SetAlignToXAxis();
    rightLine->SetAlignToXAxis();
}


void PVoiWidget::setColor(LineType type, int red, int green, int blue)
{
    if (type == Top)
        topLine->GetLineProperty()->SetColor(
            red / 255.0, green / 255.0, blue / 255.0);
    else if (type == Bottom)
        botLine->GetLineProperty()->SetColor(
            red / 255.0, green / 255.0, blue / 255.0);
    else if (type == Left)
        leftLine->GetLineProperty()->SetColor(
            red / 255.0, green / 255.0, blue / 255.0);
    else if (type == Right)
        rightLine->GetLineProperty()->SetColor(
            red / 255.0, green / 255.0, blue / 255.0);
}


void PVoiWidget::initPosition(vtkImageActor *actor, vtkCamera *camera,
    int margin)
{
    double *bound = actor->GetBounds();
    double *focal = camera->GetFocalPoint();
    double *pos = camera->GetPosition();
    
//     cout << bound[0] << " " << bound[1] << " " << bound[2] << " " <<
//             bound[3] << " " << bound[4] << " " << bound[5] << "\n" <<
//             focal[0] << " " << focal[1] << " " << focal[2] << "\n" <<
//             pos[0] << " " << pos[1] << " " << pos[2] << "\n\n" << flush;
    
    double high, low, left, right;
    
    switch(plane)
    {
        case XY:
            topXY = (focal[2] + pos[2]) / 2;
            high = bound[3] - margin;
            low = bound[2] + margin;
            left = bound[0] + margin;
            right = bound[1] - margin;
            
            topLine->SetPoint1(LowBound, high, topXY);
            topLine->SetPoint2(HighBound, high, topXY);
            botLine->SetPoint1(LowBound, low, topXY);
            botLine->SetPoint2(HighBound, low, topXY);
            leftLine->SetPoint1(left, LowBound, topXY);
            leftLine->SetPoint2(left, HighBound, topXY);
            rightLine->SetPoint1(right, LowBound, topXY);
            rightLine->SetPoint2(right, HighBound, topXY);
            break;
        
        case YZ:
            topYZ = (focal[0] + pos[0]) / 2;
            low = bound[5] - margin;
            high = bound[4] + margin;
            right = bound[2] + margin;
            left = bound[3] - margin;
            
            topLine->SetPoint1(topYZ, LowBound, high);
            topLine->SetPoint2(topYZ, HighBound, high);
            botLine->SetPoint1(topYZ, LowBound, low);
            botLine->SetPoint2(topYZ, HighBound, low);
            leftLine->SetPoint1(topYZ, left, LowBound);
            leftLine->SetPoint2(topYZ, left, HighBound);
            rightLine->SetPoint1(topYZ, right, LowBound);
            rightLine->SetPoint2(topYZ, right, HighBound);
            break;
            
        case XZ:
            topXZ = (focal[1] + pos[1]) / 2;
            low = bound[5] - margin;
            high = bound[4] + margin;
            left = bound[0] + margin;
            right = bound[1] - margin;
            
            topLine->SetPoint1(LowBound, topXZ, high);
            topLine->SetPoint2(HighBound, topXZ, high);
            botLine->SetPoint1(LowBound, topXZ, low);
            botLine->SetPoint2(HighBound, topXZ, low);
            leftLine->SetPoint1(left, topXZ, LowBound);
            leftLine->SetPoint2(left, topXZ, HighBound);
            rightLine->SetPoint1(right, topXZ, LowBound);
            rightLine->SetPoint2(right, topXZ, HighBound);
            break;
            
        default:
            break;
    }
}


void PVoiWidget::setInteractor(vtkRenderWindowInteractor *inter)
{
    interactor = inter;
    topLine->SetInteractor(inter);
    botLine->SetInteractor(inter);
    leftLine->SetInteractor(inter);
    rightLine->SetInteractor(inter);
}


// getBounds: Get coordinates of bounding box
// bound[0] = min 1st coord, bound[1] = max 1st coord,
// bound[2] = min 2nd coord, bound[3] = max 2nd coord.
// XY plane: 1st coord = x, 2nd coord = y
// YZ plane: 1st coord = y, 2nd coord = z
// XZ plane: 1st coord = x, 2nd coord = z

double *PVoiWidget::getBounds()
{
    static double bound[4];
    
    if (plane == XY)
    {
        bound[0] = leftLine->GetPoint1()[0];
        bound[1] = rightLine->GetPoint1()[0];
        bound[2] = topLine->GetPoint1()[1];
        bound[3] = botLine->GetPoint1()[1];
    }
    else if (plane == YZ)
    {
        bound[0] = rightLine->GetPoint1()[1];
        bound[1] = leftLine->GetPoint1()[1];
        bound[2] = topLine->GetPoint1()[2];
        bound[3] = botLine->GetPoint1()[2];
    }
    else if (plane == XZ)
    {
        bound[0] = leftLine->GetPoint1()[0];
        bound[1] = rightLine->GetPoint1()[0];
        bound[2] = topLine->GetPoint1()[2];
        bound[3] = botLine->GetPoint1()[2];
    }
    
    return bound;
}


void PVoiWidget::setTopLine(double pos)
{
    double p1[3], p2[3];

    if (plane == XY)
    {
        p1[0] = LowBound;
        p2[0] = HighBound;
        p1[1] = p2[1] = pos;
        p1[2] = p2[2] = topXY;
    }
    else if (plane == YZ)
    {
        p1[0] = p2[0] = topYZ;
        p1[1] = LowBound;
        p2[1] = HighBound;
        p1[2] = p2[2] = pos;
        
    }
    else if (plane == XZ)
    {
        p1[0] = LowBound;
        p2[0] = HighBound;
        p1[1] = p2[1] = topXZ;
        p1[2] = p2[2] = pos;
    }
    else
        return;
    
    topLine->SetPoint1(p1);
    topLine->SetPoint2(p2);
    update();
}


void PVoiWidget::setBottomLine(double pos)
{
    double p1[3], p2[3];
    
    if (plane == XY)
    {
        p1[0] = LowBound;
        p2[0] = HighBound;
        p1[1] = p2[1] = pos;
        p1[2] = p2[2] = topXY;
    }
    else if (plane == YZ)
    {
        p1[0] = p2[0] = topYZ;
        p1[1] = LowBound;
        p2[1] = HighBound;
        p1[2] = p2[2] = pos;
        
    }
    else if (plane == XZ)
    {
        p1[0] = LowBound;
        p2[0] = HighBound;
        p1[1] = p2[1] = topXZ;
        p1[2] = p2[2] = pos;
    }
    else
        return;
    
    botLine->SetPoint1(p1);
    botLine->SetPoint2(p2);
    update();
}


void PVoiWidget::setLeftLine(double pos)
{
    double p1[3], p2[3];
            
    if (plane == XY)
    {
        p1[0] = p2[0] = pos;
        p1[1] = LowBound;
        p2[1] = HighBound;
        p1[2] = p2[2] = topXY;
    }
    else if (plane == YZ)
    {
        p1[0] = p2[0] = topYZ;
        p1[1] = p2[1] = pos;
        p1[2] = LowBound;
        p2[2] = HighBound;
    }
    else if (plane == XZ)
    {
        p1[0] = p2[0] = pos;
        p1[1] = p2[1] = topXZ;
        p1[2] = LowBound;
        p2[2] = HighBound;
    }
    else
        return;
        
    leftLine->SetPoint1(p1);
    leftLine->SetPoint2(p2);    
    update();
}


void PVoiWidget::setRightLine(double pos)
{
    double p1[3], p2[3];
    
    if (plane == XY)
    {
        p1[0] = p2[0] = pos;
        p1[1] = LowBound;
        p2[1] = HighBound;
        p1[2] = p2[2] = topXY;
    }
    else if (plane == YZ)
    {
        p1[0] = p2[0] = topYZ;
        p1[1] = p2[1] = pos;
        p1[2] = LowBound;
        p2[2] = HighBound;
    }
    else if (plane == XZ)
    {
        p1[0] = p2[0] = pos;
        p1[1] = p2[1] = topXZ;
        p1[2] = LowBound;
        p2[2] = HighBound;
    }
    else
        return;
        
    rightLine->SetPoint1(p1);
    rightLine->SetPoint2(p2);
    update();
}


void PVoiWidget::update()
{
    interactor->Render();
}


// Slot functions

void PVoiWidget::show(bool option)
{
    int flag;
    
    if (option)
        flag = 1;
    else
        flag = 0;
        
    topLine->SetEnabled(flag);
    botLine->SetEnabled(flag);
    leftLine->SetEnabled(flag);
    rightLine->SetEnabled(flag);
}


void PVoiWidget::show()
{
    show(true);
}


void PVoiWidget::hide()
{
    show(false);
}


//--- PVoiWidgetCallback functions

PVoiWidgetCallback::PVoiWidgetCallback()
{
}


PVoiWidgetCallback::~PVoiWidgetCallback()
{
}


void PVoiWidgetCallback::Execute(vtkObject *caller,
    unsigned long eventId, void *callData)
{   
    switch(host->plane)
    {
        case PVoiWidget::XY:
            if (caller == host->topLine)
                emit host->topLineChanged(host->topLine->GetPoint1()[1]);
            else if (caller == host->botLine)
                emit host->bottomLineChanged(host->botLine->GetPoint1()[1]);
            else if (caller == host->leftLine)
                emit host->leftLineChanged(host->leftLine->GetPoint1()[0]);
            else if (caller == host->rightLine)
                emit host->rightLineChanged(host->rightLine->GetPoint1()[0]);
            break;
            
        case PVoiWidget::YZ:
            if (caller == host->topLine)
                emit host->topLineChanged(host->topLine->GetPoint1()[2]);
            else if (caller == host->botLine)
                emit host->bottomLineChanged(host->botLine->GetPoint1()[2]);
            else if (caller == host->leftLine)
                emit host->leftLineChanged(host->leftLine->GetPoint1()[1]);
            else if (caller == host->rightLine)
                emit host->rightLineChanged(host->rightLine->GetPoint1()[1]);
            break;
            
        case PVoiWidget::XZ:
            if (caller == host->topLine)
                emit host->topLineChanged(host->topLine->GetPoint1()[2]);
            else if (caller == host->botLine)
                emit host->bottomLineChanged(host->botLine->GetPoint1()[2]);
            else if (caller == host->leftLine)
                emit host->leftLineChanged(host->leftLine->GetPoint1()[0]);
            else if (caller == host->rightLine)
                emit host->rightLineChanged(host->rightLine->GetPoint1()[0]);
            break;
            
        default:
            break;
    }
}
