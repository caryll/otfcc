#ifndef CARYLL_SFNT_BUILDER_H
#define CARYLL_SFNT_BUILDER_H

#include <support/util.h>

typedef struct {
	int tag;
	uint32_t length;
	uint32_t checksum;
	caryll_buffer *buffer;
	UT_hash_handle hh;
} caryll_SFNTTableEntry;

typedef struct {
	uint32_t count;
	uint32_t header;
	caryll_SFNTTableEntry *tables;
	caryll_Options *options;
} caryll_SFNTBuilder;

caryll_SFNTBuilder *caryll_new_SFNTBuilder(uint32_t header, caryll_Options *options);
void caryll_pushTableToSfntBuilder(caryll_SFNTBuilder *builder, uint32_t tag, caryll_buffer *buffer);
void caryll_delete_SFNTBuilder(caryll_SFNTBuilder *builder);

caryll_buffer *caryll_serializeSFNT(caryll_SFNTBuilder *builder);

#endif
