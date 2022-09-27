#pragma once

#include <QObject>
#include <QImage>

class ImageProcess : public QObject
{
    Q_OBJECT
    /* Base class for all image process types*/
public:
    //Default order in list is the default priority, i.e. first will be executed first by default
    enum ProcessType {BlackBarRemoval, SampleScale, LinearScale, CubicScale, LowPassFilter}; Q_ENUM(ProcessType);

    ImageProcess();
    ~ImageProcess();

    //Returns the type
    virtual ProcessType type() const;

    //Virtual function that processes an input image into an output
    virtual QImage process(QImage inputImage);
};

