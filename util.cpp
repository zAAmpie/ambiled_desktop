#include "util.h"
#include <algorithm>

//Clamp a byte value to an upper and lower bound
inline BYTE clamp_byte(BYTE val, BYTE low, BYTE high)
{
    return std::min(high, std::max(val, low));
};

//Reverse copy memory to another destination. Pitch controls bytes that are immutable, e.g. 4 bytes in an integer array
inline void* revmemcpy(void* dest, const void* src, size_t len, size_t pitch)
{
    char* d = (char*)dest + len - 1;
    const char* s = (const char *)src;
    while (len -= pitch)
    {
        d -= pitch;
        for (size_t i = 0; i < pitch; ++i)
            *d++ = *s++;
    }
    return dest;
}
