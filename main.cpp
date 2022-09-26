#include "ambiled.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AmbiLED w;
    w.show();
    return a.exec();
}
