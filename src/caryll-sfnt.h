#ifndef CARYLL_SFNT_H
#define CARYLL_SFNT_H

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

caryll_sfnt *caryll_read_sfnt(const char *path);
void caryll_delete_sfnt(caryll_sfnt *font);
caryll_piece shift_piece(caryll_piece piece, uint32_t delta);

#endif
