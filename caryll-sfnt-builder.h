#include "support/util.h"
typedef struct {
	int tag;
	uint32_t length;
	uint32_t checksum;
	caryll_buffer *buffer;
	UT_hash_handle hh;
} sfnt_builder_entry;
typedef struct {
	uint32_t count;
	sfnt_builder_entry *tables;
} sfnt_builder;

sfnt_builder *sfnt_builder_new();
void sfnt_builder_push_table(sfnt_builder *builder, uint32_t tag, caryll_buffer *buffer);
void sfnt_builder_delete(sfnt_builder *builder);
