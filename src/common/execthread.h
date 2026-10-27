#pragma once

#include <QObject>
#include <QThread>

//Super basic class for constructing a QThread which has an event loop
class ExecThread : public QThread
{
    Q_OBJECT

public:
    //Constructor
    ExecThread(qint32 id = 0, QObject *parent = nullptr);

    void usleep(unsigned long time){
        QThread::usleep(time);}
    void sleep(unsigned long time){
        QThread::sleep(time);}
    void msleep(unsigned long time){
        QThread::msleep(time);}

    qint32 id(){return pID;}
    void setID(qint32 id){pID = id;}

protected:
    //Main EXEC function
    void run();
private:
    //ID
    qint32 pID;
};

