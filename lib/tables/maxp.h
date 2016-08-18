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

table_maxp *caryll_new_maxp();
table_maxp *caryll_read_maxp(caryll_packet packet);
void caryll_maxp_to_json(table_maxp *table, json_value *root, const caryll_options *options);
table_maxp *caryll_maxp_from_json(json_value *root, const caryll_options *options);
caryll_buffer *caryll_write_maxp(table_maxp *maxp, const caryll_options *options);
#endif
