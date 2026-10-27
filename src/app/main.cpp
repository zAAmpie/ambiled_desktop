#include "ambiled.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false); //Closing to tray should not exit the app - only the tray's Quit action should
    AmbiLED w;
    w.show();
    return a.exec();
}
