#ifndef CARYLL_SFNT_H
#define CARYLL_SFNT_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
	uint32_t tag;
	uint32_t checkSum;
	uint32_t offset;
	uint32_t length;
	uint8_t *data;
} caryll_PacketPiece;

typedef struct {
	uint32_t sfnt_version;
	uint16_t numTables;
	uint16_t searchRange;
	uint16_t entrySelector;
	uint16_t rangeShift;
	caryll_PacketPiece *pieces;
} caryll_Packet;

typedef struct {
	uint32_t type;
	uint32_t count;
	uint32_t *offsets;
	caryll_Packet *packets;
} caryll_SplineFontContainer;

caryll_SplineFontContainer *caryll_read_SFNT(FILE *file);
void caryll_delete_SFNT(caryll_SplineFontContainer *font);

#endif
