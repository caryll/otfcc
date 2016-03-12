#pragma once

#include <stdint.h>
#include "caryll-font.h"

typedef struct {
	uint32_t version;
	uint32_t FontNumber;
	uint16_t Pitch;
	uint16_t xHeight;
	uint16_t Style;
	uint16_t TypeFamily;
	uint16_t CapHeight;
	uint16_t SymbolSet;
	int8_t Typeface[16];
	int8_t CharacterComplement[8];
	int8_t FileName[6];
	int8_t StrokeWeight;
	int8_t WidthType;
	uint8_t SerifStyle;
	uint8_t pad;
} table_PCLT;

void caryll_read_PCLT(caryll_font *font, caryll_packet packet);
