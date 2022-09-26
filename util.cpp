#include "util.h"
#include <algorithm>

inline BYTE clamp_byte(BYTE val, BYTE low, BYTE high)
{
    return std::min(high, std::max(val, low));
};
