#pragma once

#include "imageprocess.h"

class FilterProcess : public Process
{
    /* Filters given lines */
public:
    FilterProcess();
    ProcessType type() const;
};

