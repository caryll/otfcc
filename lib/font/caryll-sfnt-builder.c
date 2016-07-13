#include "caryll-sfnt-builder.h"

#ifndef MAIN_VER
#define MAIN_VER 0
#endif
#ifndef SECONDARY_VER
#define SECONDARY_VER 0
#endif
#ifndef PATCH_VER
#define PATCH_VER 0
#endif

static uint32_t buf_checksum(caryll_buffer *buffer) {
	uint32_t actualLength = (uint32_t)buflen(buffer);
	buflongalign(buffer);
	uint32_t sum = 0;
	{
		uint32_t *start = (uint32_t *)buffer->data;
		uint32_t *end = start + ((actualLength + 3) & ~3) / sizeof(uint32_t);
		while (start < end) { sum += caryll_endian_convert32(*start++); }
	}
	return sum;
}

static sfnt_builder_entry *createSegment(uint32_t tag, caryll_buffer *buffer) {
	sfnt_builder_entry *table = malloc(sizeof(sfnt_builder_entry));
	table->tag = tag;
	table->length = (uint32_t)buflen(buffer);
	buflongalign(buffer);
	table->buffer = buffer;

	uint32_t sum = 0;
	{
		uint32_t *start = (uint32_t *)buffer->data;
		uint32_t *end = start + ((table->length + 3) & ~3) / sizeof(uint32_t);
		while (start < end) { sum += caryll_endian_convert32(*start++); }
	}
	table->checksum = sum;
	return table;
}

sfnt_builder *new_sfnt_builder(uint32_t header) {
	sfnt_builder *builder = malloc(sizeof(sfnt_builder));
	builder->count = 0;
	builder->header = header;
	builder->tables = NULL;
	return builder;
}

void delete_sfnt_builder(sfnt_builder *builder) {
	if (!builder) return;
	sfnt_builder_entry *item, *tmp;
	HASH_ITER(hh, builder->tables, item, tmp) {
		HASH_DEL(builder->tables, item);
		buffree(item->buffer);
		free(item);
	}
	free(builder);
}

void sfnt_builder_push_table(sfnt_builder *builder, uint32_t tag, caryll_buffer *buffer) {
	if (!builder) return;
	sfnt_builder_entry *item;
	HASH_FIND_INT(builder->tables, &tag, item);
	if (!item) {
		item = createSegment(tag, buffer);
		HASH_ADD_INT(builder->tables, tag, item);
	} else {
		buffree(buffer);
	}
}

static int byTag(sfnt_builder_entry *a, sfnt_builder_entry *b) { return (a->tag - b->tag); }

caryll_buffer *sfnt_builder_serialize(sfnt_builder *builder) {
	caryll_buffer *buffer = bufnew();
	if (!builder) return buffer;
	uint16_t nTables = HASH_COUNT(builder->tables);
	uint16_t searchRange = (nTables < 16 ? 8 : nTables < 32 ? 16 : nTables < 64 ? 32 : 64) * 16;
	bufwrite32b(buffer, builder->header);
	bufwrite16b(buffer, nTables);
	bufwrite16b(buffer, searchRange);
	bufwrite16b(buffer, (nTables < 16 ? 3 : nTables < 32 ? 4 : nTables < 64 ? 5 : 6));
	bufwrite16b(buffer, nTables * 16 - searchRange);

	sfnt_builder_entry *table;
	size_t offset = 32 + nTables * 16;
	size_t headOffset = offset;
	HASH_SORT(builder->tables, byTag);
	foreach_hash(table, builder->tables) {
		// write table directory
		bufwrite32b(buffer, table->tag);
		bufwrite32b(buffer, table->checksum);
		bufwrite32b(buffer, (uint32_t)offset);
		bufwrite32b(buffer, table->length);
		size_t cp = buffer->cursor;
		bufseek(buffer, offset);
		bufwrite_buf(buffer, table->buffer);
		bufseek(buffer, cp);
		// record where the [head] is
		if (table->tag == 'head') { headOffset = offset; }
		offset += buflen(table->buffer);
	}

	// we are right after the table directory
	// add copyright information
	sds copyright =
	    sdscatprintf(sdsempty(), "-- By OTFCC %d.%d.%d --", MAIN_VER, SECONDARY_VER, PATCH_VER);
	sdsgrowzero(copyright, 20);
	bufwrite_bytes(buffer, 20, (uint8_t *)copyright);
	sdsfree(copyright);

	// write head.checksumAdjust
	uint32_t wholeChecksum = buf_checksum(buffer);
	bufseek(buffer, headOffset + 8);
	bufwrite32b(buffer, 0xB1B0AFBA - wholeChecksum);
	return buffer;
}
