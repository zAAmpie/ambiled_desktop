#pragma once

#include "imageprocess.h"

class BlackBarProcess : public Process
{
    /* Removes black bars from widescreen content */
public:
    BlackBarProcess();
    ProcessType type() const;
};


