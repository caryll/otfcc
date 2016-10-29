#ifndef CARYLL_INCLUDE_TABLE_GASP_H
#define CARYLL_INCLUDE_TABLE_GASP_H

#include "table-common.h"

typedef struct {
	uint16_t rangeMaxPPEM;
	bool dogray;
	bool gridfit;
	bool symmetric_smoothing;
	bool symmetric_gridfit;
} gasp_Record;
typedef struct {
	uint16_t version;
	uint16_t numRanges;
	gasp_Record *records;
} table_gasp;

#endif
