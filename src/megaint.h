#include <stdint.h>

#ifndef TYPES_H
#define TYPES_H

// Use __MEGA65__ instead of __CC65__ incase we use a different compiler in the future
#if !defined(__MEGA65__) && defined(__CC65__)
#define __MEGA65__
#endif

#ifdef __MEGA65__
typedef uint8_t* charptr_t;
typedef uint16_t shortptr_t;
typedef uint32_t longptr_t;
typedef uint32_t ptr_t;

#else // we're on a PC so use correct C/C++ pointer types

typedef uint8_t* charptr_t;
typedef uint16_t shortptr_t;
typedef uintptr_t longptr_t;
typedef uint8_t* ptr_t;

#endif // __MEGA65__

#endif // TYPES_H