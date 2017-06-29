#ifndef CARYLL_INCLUDE_OTFCC_PRIMITIVES_H
#define CARYLL_INCLUDE_OTFCC_PRIMITIVES_H

#include <stdint.h>
#include <stdbool.h>
#include <float.h>

typedef int16_t f2dot14;
typedef int32_t f16dot16;

typedef uint16_t glyphid_t;    // Glyph index
typedef uint16_t glyphclass_t; // Glyph class
typedef uint16_t glyphsize_t;  // GASP glyph size
typedef uint16_t tableid_t;    // Table/Font structure index
typedef uint16_t colorid_t;    // Color index
typedef uint16_t shapeid_t;    // Shape index
typedef uint16_t cffsid_t;     // CFF/CFF2 String index
typedef uint32_t arity_t;      // CFF Arity/Stack depth
typedef uint32_t unicode_t;    // Unicode

typedef double pos_t; // Position
#define POS_MAX FLT_MAX
#define POS_MIN FLT_MIN

typedef double length_t; // Length

double otfcc_from_f2dot14(const f2dot14 x);
int16_t otfcc_to_f2dot14(const double x);
double otfcc_from_fixed(const f16dot16 x);
f16dot16 otfcc_to_fixed(const double x);

#endif
