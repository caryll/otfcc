#ifndef CARYLL_CFF_VALUE_H
#define CARYLL_CFF_VALUE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/util.h>

typedef enum {
	CFF_OPERATOR = 1,
	CS2_OPERATOR = 1,
	CFF_INTEGER = 2,
	CS2_OPERAND = 2,
	CFF_DOUBLE = 3,
	CS2_FRACTION = 3
} CFF_Value_Type;

typedef struct {
	CFF_Value_Type t;
	union {
		int32_t i;
		double d;
	};
} CFF_Value;

double cffnum(CFF_Value v);

#endif
