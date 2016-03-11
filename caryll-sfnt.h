#pragma once

#include <stdint.h>

typedef struct {
	uint32_t tag;
	uint32_t checkSum;
	uint32_t offset;
	uint32_t length;
	uint8_t *data;
} caryll_piece;

typedef struct {
	uint32_t sfnt_version;
	uint16_t numTables;
	uint16_t searchRange;
	uint16_t entrySelector;
	uint16_t rangeShift;
	caryll_piece *pieces;
} caryll_packet;

typedef struct {
	uint32_t type;
	uint32_t count;
	uint32_t *offsets;
	caryll_packet *packets;
} caryll_sfnt;

caryll_sfnt *caryll_sfnt_open(const char *path);
void caryll_sfnt_close(caryll_sfnt *font);
