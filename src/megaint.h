#include <stdint.h>

#ifndef TYPES_H
#define TYPES_H
#ifdef __CC65__
#define __MEGA65__

typedef uint8_t* charptr_t;
typedef uint16_t shortptr_t;
typedef uint32_t longptr_t;
typedef uint32_t ptr_t;

#else // we're on a PC so use correct C pointer types

typedef uint8_t* charptr_t;
typedef uint16_t shortptr_t;
typedef intptr_t longptr_t;
typedef uint8_t* ptr_t;

#endif // __CC65__
#endif // TYPES_H