#ifndef CARYLL_INCLUDE_OTFCC_PRIMITIVES_H
#define CARYLL_INCLUDE_OTFCC_PRIMITIVES_H

#include <stdint.h>
#include <stdbool.h>

typedef int16_t f2dot14;
typedef int32_t f16dot16;

typedef uint16_t glyphid_t;
typedef uint16_t glyphclass_t;
typedef uint16_t tableid_t;
typedef uint16_t shapeid_t;
typedef uint16_t cffsid_t;
typedef uint32_t arity_t; // CFF2 may support large arity

typedef double pos_t;
typedef double length_t;

#endif
