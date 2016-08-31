#ifndef CARYLL_TABLES_MAXP_H
#define CARYLL_TABLES_MAXP_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

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

table_maxp *table_new_maxp();
table_maxp *table_read_maxp(caryll_Packet packet);
void table_dump_maxp(table_maxp *table, json_value *root, const caryll_Options *options);
table_maxp *table_parse_maxp(json_value *root, const caryll_Options *options);
caryll_buffer *table_build_maxp(table_maxp *maxp, const caryll_Options *options);
#endif
