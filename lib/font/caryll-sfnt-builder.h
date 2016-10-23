#ifndef CARYLL_SFNT_BUILDER_H
#define CARYLL_SFNT_BUILDER_H

#include "support/util.h"

typedef struct {
	int tag;
	uint32_t length;
	uint32_t checksum;
	caryll_Buffer *buffer;
	UT_hash_handle hh;
} caryll_SFNTTableEntry;

typedef struct {
	uint32_t count;
	uint32_t header;
	caryll_SFNTTableEntry *tables;
	otfcc_Options *options;
} caryll_SFNTBuilder;

caryll_SFNTBuilder *caryll_new_SFNTBuilder(uint32_t header, otfcc_Options *options);
void caryll_pushTableToSfntBuilder(caryll_SFNTBuilder *builder, uint32_t tag, caryll_Buffer *buffer);
void caryll_delete_SFNTBuilder(caryll_SFNTBuilder *builder);

caryll_Buffer *caryll_serializeSFNT(caryll_SFNTBuilder *builder);

#endif
