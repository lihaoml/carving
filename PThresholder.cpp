/* PThresholder.cpp

   Interactive thresholding tool.

   Copyright 2012, 2103, National University of Singapore
   Author: Leow Wee Kheng
*/

#include "PThresholder.h"
#include <QtGui>

using namespace std;


PThresholder::PThresholder()
{
    threshold = vtkImageThreshold::New();
    hasInput = false;
    
    lowerMin = -50;
    lowerMax = 512;
    upperMin = 128;
    upperMax = 1024;
    lower = 0;
    upper = 1024;
    fill = -500;
    threshold->ReplaceInOn();
    threshold->SetInValue(fill);
    threshold->ThresholdByLower(lower);

    type = Lower;
    createWidgets();
}


PThresholder::~PThresholder()
{
    threshold->Delete();
}


void PThresholder::setInputConnection(vtkAlgorithmOutput *input)
{
    if (!input)
        return;
        
    threshold->SetInputConnection(input);
    hasInput = true;
}


vtkAlgorithmOutput* PThresholder::getOutputPort()
{
    return threshold->GetOutputPort();
}


vtkImageData *PThresholder::getOutput()
{
    return threshold->GetOutput();
}


void PThresholder::apply()
{
    if (!hasInput)
        return;
    
    if (type == Lower)
        threshold->ThresholdByLower(lower);
    else if (type == Upper)
        threshold->ThresholdByUpper(upper);
    else // type == Between
        threshold->ThresholdBetween(lower, upper);

    emit updated();
}


void PThresholder::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit closed();
}


void PThresholder::createWidgets()
{
    setWindowTitle("Threshold Dialog");
        
    QGroupBox *gbox = new QGroupBox(tr("threshold type"));
    QRadioButton *lowerButton = new QRadioButton(gbox);
    lowerButton->setText(tr("lower"));
    QRadioButton *betweenButton = new QRadioButton(gbox);
    betweenButton->setText(tr("between"));
    QRadioButton *upperButton = new QRadioButton(gbox);
    upperButton->setText(tr("upper"));
    lowerButton->setChecked(true); // Default;

    QHBoxLayout *choiceBox = new QHBoxLayout(gbox);
    choiceBox->addWidget(lowerButton);
    choiceBox->addWidget(betweenButton);
    choiceBox->addWidget(upperButton);
    connect(lowerButton, SIGNAL(clicked()), this,
        SLOT(thresholdLower()));
    connect(betweenButton, SIGNAL(clicked()), this,
        SLOT(thresholdBetween()));
    connect(upperButton, SIGNAL(clicked()), this,
        SLOT(thresholdUpper()));
        
    QLabel *upperRangeLabel1 = new QLabel("upper");
    QLabel *upperRangeLabel2 = new QLabel("range");
    upperMinBox = new QLineEdit;
    upperMinBox->setMaxLength(5);
    upperMinBox->setFixedWidth(50);
    upperMinBox->setText(QString("%1").arg(upperMin));
    connect(upperMinBox, SIGNAL(textEdited(const QString &)), this,
        SLOT(setUpperMin(const QString &)));
    upperMaxBox = new QLineEdit;
    upperMaxBox->setMaxLength(5);
    upperMaxBox->setFixedWidth(50);
    upperMaxBox->setText(QString("%1").arg(upperMax));
    connect(upperMaxBox, SIGNAL(textEdited(const QString &)), this,
        SLOT(setUpperMax(const QString &)));  
        
    QHBoxLayout *upperRangeBox = new QHBoxLayout;
    upperRangeBox->addWidget(upperMinBox);
    upperRangeBox->addStretch();
    upperRangeBox->addWidget(upperMaxBox);

    QLabel *upperLabel = new QLabel("upper");
    upperBox = new QSpinBox;
    upperBox->setRange(upperMin, upperMax);
    upperSlider = new QSlider(Qt::Horizontal);
    upperSlider->setRange(upperMin, upperMax);
    connect(upperBox, SIGNAL(valueChanged(int)), upperSlider,
        SLOT(setValue(int)));
    connect(upperSlider, SIGNAL(valueChanged(int)), upperBox,
        SLOT(setValue(int)));
    connect(upperBox, SIGNAL(valueChanged(int)), this,
        SLOT(setUpper(int)));
    connect(upperSlider, SIGNAL(valueChanged(int)), this,
        SLOT(setUpper(int)));
    upperBox->setValue(upperMax);
    
    QLabel *lowerRangeLabel1 = new QLabel("lower");
    QLabel *lowerRangeLabel2 = new QLabel("range");
    lowerMinBox = new QLineEdit;
    lowerMinBox->setMaxLength(5);
    lowerMinBox->setFixedWidth(50);
    lowerMinBox->setText(QString("%1").arg(lowerMin));
    connect(lowerMinBox, SIGNAL(textEdited(const QString &)), this,
        SLOT(setLowerMin(const QString &)));
    lowerMaxBox = new QLineEdit;
    lowerMaxBox->setMaxLength(5);
    lowerMaxBox->setFixedWidth(50);
    lowerMaxBox->setText(QString("%1").arg(lowerMax));
    connect(lowerMaxBox, SIGNAL(textEdited(const QString &)), this,
        SLOT(setLowerMax(const QString &)));

    QHBoxLayout *lowerRangeBox = new QHBoxLayout;
    lowerRangeBox->addWidget(lowerMinBox);
    lowerRangeBox->addStretch();
    lowerRangeBox->addWidget(lowerMaxBox);
    
    QLabel *lowerLabel = new QLabel("lower");
    lowerBox = new QSpinBox;
    lowerBox->setRange(lowerMin, lowerMax);
    lowerSlider = new QSlider(Qt::Horizontal);
    lowerSlider->setRange(lowerMin, lowerMax);
    connect(lowerBox, SIGNAL(valueChanged(int)), lowerSlider,
        SLOT(setValue(int)));
    connect(lowerSlider, SIGNAL(valueChanged(int)), lowerBox,
        SLOT(setValue(int)));
    connect(lowerBox, SIGNAL(valueChanged(int)), this,
        SLOT(setLower(int)));
    connect(lowerSlider, SIGNAL(valueChanged(int)), this,
        SLOT(setLower(int)));
    lowerBox->setValue(lower);

    QLabel *fillLabel = new QLabel("fill in");
    fillBox = new QLineEdit;
    fillBox->setMaxLength(5);
    fillBox->setFixedWidth(50);
    connect(fillBox, SIGNAL(textEdited(const QString &)), this,
        SLOT(setFill(const QString &)));
    fillBox->setText("-500");
        
    QPushButton *closeButton = new QPushButton("close");
    connect(closeButton, SIGNAL(clicked()), this, SLOT(hide()));
    connect(closeButton, SIGNAL(clicked()), this, SIGNAL(closed()));
    QHBoxLayout *closeBox = new QHBoxLayout;
    closeBox->addStretch();
    closeBox->addWidget(closeButton);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(upperRangeLabel1, 0, 0);
    grid->addWidget(upperRangeLabel2, 0, 1);
    grid->addLayout(upperRangeBox, 0, 2);
    grid->addWidget(upperLabel, 1, 0);
    grid->addWidget(upperBox, 1, 1);
    grid->addWidget(upperSlider, 1, 2);
    grid->addWidget(lowerRangeLabel1, 2, 0);
    grid->addWidget(lowerRangeLabel2, 2, 1);
    grid->addLayout(lowerRangeBox, 2, 2);
    grid->addWidget(lowerLabel, 3, 0);
    grid->addWidget(lowerBox, 3, 1);
    grid->addWidget(lowerSlider, 3, 2);
    grid->addWidget(fillLabel, 4, 0);
    grid->addWidget(fillBox, 4, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(gbox);
    mainLayout->addLayout(grid);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(closeBox);
    setLayout(mainLayout);
    setFixedSize(300, 300);
}


// Slot functions

void PThresholder::thresholdLower()
{
    type = Lower;
    apply();
}


void PThresholder::thresholdBetween()
{
    type = Between;
    apply();
}


void PThresholder::thresholdUpper()
{
    type = Upper;
    apply();
}


void PThresholder::setUpperMin(const QString &text)
{
    bool ok;   
    int value = text.trimmed().toDouble(&ok);
    
    if (ok)
    {
        upperMin = value;
        upperMinBox->setText(QString("%1").arg(value));
        upperBox->setRange(upperMin, upperMax);
        upperSlider->setRange(upperMin, upperMax);

        if (upper < upperMin)
        {
            upper = upperMin;
            upperBox->setValue(upper);
            apply();
        }
    }
    else
    {
        upperMinBox->setText(QString("%1").arg((int) upperMin));
    }
}


void PThresholder::setUpperMax(const QString &text)
{
    bool ok;   
    int value = text.trimmed().toDouble(&ok);
    
    if (ok)
    {
        upperMax = value;
        upperMaxBox->setText(QString("%1").arg(value));
        upperBox->setRange(upperMin, upperMax);
        upperSlider->setRange(upperMin, upperMax);

        if (upper > upperMax)
        {
            upper = upperMax;
            upperBox->setValue(upper);
            apply();
        }
    }
    else
    {
        upperMaxBox->setText(QString("%1").arg((int) upperMax));
    }
}


void PThresholder::setUpper(int value)
{
    upper = value;
    apply();
}


void PThresholder::setLowerMin(const QString &text)
{
    bool ok;   
    int value = text.trimmed().toDouble(&ok);
    
    if (ok)
    {
        lowerMin = value;
        lowerMinBox->setText(QString("%1").arg(value));
        lowerBox->setRange(lowerMin, lowerMax);
        lowerSlider->setRange(lowerMin, lowerMax);

        if (lower < lowerMin)
        {
            lower = lowerMin;
            lowerBox->setValue(lower);
            apply();
        }
    }
    else
    {
        lowerMinBox->setText(QString("%1").arg((int) lowerMin));
    }
}


void PThresholder::setLowerMax(const QString &text)
{
    bool ok;   
    int value = text.trimmed().toDouble(&ok);
    
    if (ok)
    {
        lowerMax = value;
        lowerMaxBox->setText(QString("%1").arg(value));
        lowerBox->setRange(lowerMin, lowerMax);
        lowerSlider->setRange(lowerMin, lowerMax);

        if (lower > lowerMax)
        {
            lower = lowerMax;
            lowerBox->setValue(lower);
            apply();
        }
    }
    else
    {
        lowerMaxBox->setText(QString("%1").arg((int) lowerMax));
    }
}


void PThresholder::setLower(int value)
{
    lower = value;
    apply();
}


void PThresholder::setFill(const QString &text)
{
    bool ok;   
    int value = text.trimmed().toDouble(&ok);
    
    if (ok)
    {
        fill = value;
        fillBox->setText(QString("%1").arg(value));
        threshold->SetInValue(fill);
        apply();
    }
    else
    {
        fillBox->setText(QString("%1").arg((int) fill));
    }
}
