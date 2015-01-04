// main.cpp

#include <QApplication>
#include "PBrainExtractor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    PBrainExtractor viewer;
    viewer.show();
    return app.exec();
}

