/* PThresholder.h

   Interactive thresholding tool.

   Copyright 2012, 2103, National University of Singapore
   Author: Leow Wee Kheng
*/

#ifndef PTHRESHOLDER_H
#define PTHRESHOLDER_H

#include <QObject>
#include <QVariant>
#include <QSpinBox>
#include <QSlider>
#include <QLineEdit>
#include "vtkImageThreshold.h"


class PThresholder: public QWidget
{
    Q_OBJECT

public:
    PThresholder();
    ~PThresholder();

    void setInputConnection(vtkAlgorithmOutput *input);
    vtkAlgorithmOutput *getOutputPort();
    vtkImageData *getOutput();
    void apply();
    
protected:
    void closeEvent(QCloseEvent *event);
    
signals:
    void updated();
    void closed();

protected slots:
    void thresholdLower();
    void thresholdBetween();
    void thresholdUpper();
    void setUpperMin(const QString &text);
    void setUpperMax(const QString &text);
    void setUpper(int value);
    void setLowerMin(const QString &text);
    void setLowerMax(const QString &text);
    void setLower(int value);
    void setFill(const QString &text);

protected:
    vtkImageThreshold *threshold;
    bool hasInput;
    double upperMin, upperMax;
    double lowerMin, lowerMax;
    double upper;
    double lower;
    double fill;
    enum Type {Lower, Between, Upper};
    Type type;

    QLineEdit *upperMinBox;
    QLineEdit *upperMaxBox;
    QSpinBox  *upperBox;
    QLineEdit *lowerMinBox;
    QLineEdit *lowerMaxBox;
    QSpinBox  *lowerBox;
    QLineEdit *fillBox;
    QSlider *upperSlider;
    QSlider *lowerSlider;

    void createWidgets();
};

#endif
