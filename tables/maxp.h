#ifndef CARYLL_TABLES_MAXP_H
#define CARYLL_TABLES_MAXP_H

#include <stdint.h>
#include "../caryll-font.h"

typedef struct {
	// Maximum profile
	uint32_t version;
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

void caryll_read_maxp(caryll_font *font, caryll_packet packet);

#endif
