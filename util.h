#pragma once

#define PI 3.14159265
#define INT_TO_BYTE(intValue) static_cast<BYTE>(intValue)

//Clamp a byte value to an upper and lower bound
inline BYTE clamp_byte(BYTE val, BYTE low, BYTE high);


