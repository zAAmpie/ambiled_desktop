#pragma once

#include "types.h"
#include <cstddef>

#define PI 3.14159265
#define INT_TO_BYTE(intValue) static_cast<BYTE>(intValue)

//Clamp a byte value to an upper and lower bound
inline BYTE clamp_byte(BYTE val, BYTE low, BYTE high);

//Reverse copy memory to another destination. Pitch controls bytes that are immutable, e.g. 4 bytes in an integer array
void* revmemcpy(void* dest, const void* src, size_t len, size_t pitch = 1);
