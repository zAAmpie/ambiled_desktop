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
    //Default order in list is the default priority, i.e. first will be executed first by default.
    //Scaling comes first so everything after it works on the smaller image. Settings persist types
    //by name (QMetaEnum key), so reordering here is safe.
    enum ProcessType {SampleScale, LinearScale, CubicScale, BlackBarRemoval, LowPassFilter}; Q_ENUM(ProcessType);

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

    //Whether output depends on previous frames (not just the current input) - if so, an unchanged
    //input frame still has to be processed rather than reusing the last result
    virtual bool isStateful() const {return false;}

    //Generic configuration contract: current parameter values, and a way to update them live.
    //Subclasses with tunable parameters override both; the UI builds its config pages purely
    //from these rather than needing per-type knowledge in ProcessManager or AmbiLED.
    virtual QVariantMap parameters() const {return QVariantMap();}
    virtual void setParameters(const QVariantMap &parameters) {Q_UNUSED(parameters);}
};

