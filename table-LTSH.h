#pragma once

#include <stdint.h>

typedef struct {
	uint16_t version;
	uint16_t numGlyphs;
	uint8_t *yPels;
} table_LTSH;
