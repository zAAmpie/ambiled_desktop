#pragma once

#include "imageprocess.h"

class ScaleProcess : public Process
{
    /* Scales an image for further processing */
public:
    ScaleProcess();
    ProcessType type() const;
};

