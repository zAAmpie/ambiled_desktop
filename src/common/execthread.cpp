#include "execthread.h"

//Constructor
ExecThread::ExecThread(qint32 id, QObject *parent) : QThread(parent)
{
    pID = id;

    //Automatically destroy the thread
    QObject::connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

//Main EXEC function
void ExecThread::run()
{
    //Start the event loop
    exec();
}
