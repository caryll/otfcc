#ifndef CARYLL_INCLUDE_TABLE_MAXP_H
#define CARYLL_INCLUDE_TABLE_MAXP_H

#include "table-common.h"

typedef struct {
	// Maximum profile
	f16dot16 version;
	uint16_t numGlyphs;
	uint16_t maxPoints;
	uint16_t maxContours;
	uint16_t maxCompositePoints;
	uint16_t maxCompositeContours;
	uint16_t maxZones;
	uint16_t maxTwilightPoints;
	uint16_t maxStorage;
	uint16_t maxFunctionDefs;
	uint16_t maxInstructionDefs;
	uint16_t maxStackElements;
	uint16_t maxSizeOfInstructions;
	uint16_t maxComponentElements;
	uint16_t maxComponentDepth;
} table_maxp;

table_maxp *otfcc_newMaxp();
void otfcc_deleteMaxp(MOVE table_maxp *maxp);
table_maxp *otfcc_readMaxp(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpMaxp(const table_maxp *table, json_value *root, const otfcc_Options *options);
table_maxp *otfcc_parseMaxp(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildMaxp(const table_maxp *maxp, const otfcc_Options *options);

#endif
