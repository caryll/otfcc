#ifndef CARYLL_VF_AXIS_H
#define CARYLL_VF_AXIS_H

#include "otfcc/primitives.h"

typedef struct {
	uint32_t tag;
	pos_t minValue;
	pos_t defaultValue;
	pos_t maxValue;
	uint16_t flags;
	uint16_t axisNameID;
} VF_Axis;

#endif
