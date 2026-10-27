#include "util.h"
#include <algorithm>
#include <QDebug>

//Clamp a byte value to an upper and lower bound
BYTE clamp_byte(BYTE val, BYTE low, BYTE high)
{
    return std::min(high, std::max(val, low));
};

//Reverse copy memory to another destination. Pitch controls bytes that are immutable, e.g. 4 bytes in an integer array
void* revmemcpy(void* dest, const void* src, size_t len, size_t pitch)
{
    BYTE* d = (BYTE*)dest + len;
    const BYTE* s = (const BYTE *)src;
    while (len -= pitch)
    {
        d -= pitch;
        memcpy(d, s, pitch);
        s += pitch;
    }
    return dest;
}

