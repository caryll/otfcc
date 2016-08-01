#ifndef CARYLL_SFNT_BUILDER_H
#define CARYLL_SFNT_BUILDER_H

#include <support/util.h>

typedef struct {
	int tag;
	uint32_t length;
	uint32_t checksum;
	caryll_buffer *buffer;
	UT_hash_handle hh;
} sfnt_builder_entry;

typedef struct {
	uint32_t count;
	uint32_t header;
	sfnt_builder_entry *tables;
	caryll_options *options;
} sfnt_builder;

sfnt_builder *new_sfnt_builder(uint32_t header, caryll_options *options);
void sfnt_builder_push_table(sfnt_builder *builder, uint32_t tag, caryll_buffer *buffer);
void delete_sfnt_builder(sfnt_builder *builder);

caryll_buffer *sfnt_builder_serialize(sfnt_builder *builder);

#endif
