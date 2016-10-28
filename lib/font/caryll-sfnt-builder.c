#include "support/util.h"
#include "otfcc/sfnt-builder.h"

#ifndef MAIN_VER
#define MAIN_VER 0
#endif
#ifndef SECONDARY_VER
#define SECONDARY_VER 0
#endif
#ifndef PATCH_VER
#define PATCH_VER 0
#endif

static uint32_t buf_checksum(caryll_Buffer *buffer) {
	uint32_t actualLength = (uint32_t)buflen(buffer);
	buflongalign(buffer);
	uint32_t sum = 0;
	{
		uint32_t *start = (uint32_t *)buffer->data;
		uint32_t *end = start + ((actualLength + 3) & ~3) / sizeof(uint32_t);
		while (start < end) {
			sum += caryll_endian_convert32(*start++);
		}
	}
	return sum;
}

static caryll_SFNTTableEntry *createSegment(uint32_t tag, caryll_Buffer *buffer) {
	caryll_SFNTTableEntry *table = malloc(sizeof(caryll_SFNTTableEntry));
	table->tag = tag;
	table->length = (uint32_t)buflen(buffer);
	buflongalign(buffer);
	table->buffer = buffer;

	uint32_t sum = 0;
	{
		uint32_t *start = (uint32_t *)buffer->data;
		uint32_t *end = start + ((table->length + 3) & ~3) / sizeof(uint32_t);
		while (start < end) {
			sum += caryll_endian_convert32(*start++);
		}
	}
	table->checksum = sum;
	return table;
}

caryll_SFNTBuilder *caryll_new_SFNTBuilder(uint32_t header, otfcc_Options *options) {
	caryll_SFNTBuilder *builder = malloc(sizeof(caryll_SFNTBuilder));
	builder->count = 0;
	builder->header = header;
	builder->tables = NULL;
	builder->options = options;
	return builder;
}

void caryll_delete_SFNTBuilder(caryll_SFNTBuilder *builder) {
	if (!builder) return;
	caryll_SFNTTableEntry *item, *tmp;
	HASH_ITER(hh, builder->tables, item, tmp) {
		HASH_DEL(builder->tables, item);
		buffree(item->buffer);
		free(item);
	}
	free(builder);
}

void caryll_pushTableToSfntBuilder(caryll_SFNTBuilder *builder, uint32_t tag, caryll_Buffer *buffer) {
	if (!builder) return;
	caryll_SFNTTableEntry *item;
	const otfcc_Options *options = builder->options;
	HASH_FIND_INT(builder->tables, &tag, item);
	if (!item) {
		item = createSegment(tag, buffer);
		HASH_ADD_INT(builder->tables, tag, item);
		logProgress("OpenType table %c%c%c%c successfully built.\n", (tag >> 24) & 0xff, (tag >> 16) & 0xff,
		            (tag >> 8) & 0xff, tag & 0xff);
	} else {
		buffree(buffer);
	}
}

static int byTag(caryll_SFNTTableEntry *a, caryll_SFNTTableEntry *b) {
	return (a->tag - b->tag);
}

caryll_Buffer *caryll_serializeSFNT(caryll_SFNTBuilder *builder) {
	caryll_Buffer *buffer = bufnew();
	if (!builder) return buffer;
	uint16_t nTables = HASH_COUNT(builder->tables);
	uint16_t searchRange = (nTables < 16 ? 8 : nTables < 32 ? 16 : nTables < 64 ? 32 : 64) * 16;
	bufwrite32b(buffer, builder->header);
	bufwrite16b(buffer, nTables);
	bufwrite16b(buffer, searchRange);
	bufwrite16b(buffer, (nTables < 16 ? 3 : nTables < 32 ? 4 : nTables < 64 ? 5 : 6));
	bufwrite16b(buffer, nTables * 16 - searchRange);

	caryll_SFNTTableEntry *table;
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
	sds copyright = sdscatprintf(sdsempty(), "-- By OTFCC %d.%d.%d --", MAIN_VER, SECONDARY_VER, PATCH_VER);
	sdsgrowzero(copyright, 20);
	bufwrite_bytes(buffer, 20, (uint8_t *)copyright);
	sdsfree(copyright);

	// write head.checksumAdjust
	uint32_t wholeChecksum = buf_checksum(buffer);
	bufseek(buffer, headOffset + 8);
	bufwrite32b(buffer, 0xB1B0AFBA - wholeChecksum);
	return buffer;
}
