#pragma once

#include <QObject>
#include <QImage>
#include <QVariantMap>
#include <QPair>
#include <QList>
#include <QString>

class ImageProcess : public QObject
{
    Q_OBJECT
    /* Base class for all image process types*/
public:
    //Default order in list is the default priority, i.e. first will be executed first by default
    enum ProcessType {BlackBarRemoval, SampleScale, LinearScale, CubicScale, LowPassFilter}; Q_ENUM(ProcessType);

    //Describes one user-configurable parameter of a process type, for building generic config UI
    struct ParameterSpec
    {
        enum Kind {IntValue, Choice};

        Kind kind = IntValue;
        QString key;
        QString label;

        //IntValue
        int minimum = 0;
        int maximum = 100;
        int step = 1;
        QString suffix;

        //Choice
        QList<QPair<QString, int>> choices;
    };

    ImageProcess(QObject *parent = nullptr);
    virtual ~ImageProcess();

    //Returns the type
    virtual ProcessType type() const = 0;

    //Virtual function that processes an input image into an output
    virtual QImage process(QImage inputImage) = 0;

    //Generic configuration contract: current parameter values, and a way to update them live.
    //Subclasses with tunable parameters override both; the UI builds its config pages purely
    //from these rather than needing per-type knowledge in ProcessManager or AmbiLED.
    virtual QVariantMap parameters() const {return QVariantMap();}
    virtual void setParameters(const QVariantMap &parameters) {Q_UNUSED(parameters);}
};

