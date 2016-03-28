#include "caryll-sfnt-builder.h"
static INLINE sfnt_builder_entry *createSegment(uint32_t tag, caryll_buffer *buffer) {
	sfnt_builder_entry *table = malloc(sizeof(sfnt_builder_entry));
	table->tag = tag;
	table->length = buflen(buffer);
	buflongalign(buffer);
	table->buffer = buffer;

	uint32_t sum = 0;
	{
		uint32_t *start = (uint32_t *)buffer->s;
		uint32_t *end = start + ((table->length + 3) & ~3) / sizeof(uint32_t);
		while (start < end) {
			sum += *start++;
		}
	}
	table->checksum = sum;
	return table;
}

sfnt_builder *sfnt_builder_new(){
	sfnt_builder *builder = malloc(sizeof(sfnt_builder));
	builder->count = 0;
	builder->tables = NULL;
	return builder;
}
void sfnt_builder_delete(sfnt_builder *builder){
	sfnt_builder_entry *item, *tmp;
	HASH_ITER(hh, builder->tables, item, tmp){
		HASH_DEL(builder->tables, item);
		buffree(item->buffer);
		free(item);
	}
	free(builder);
}
void sfnt_builder_push_table(sfnt_builder *builder, uint32_t tag, caryll_buffer *buffer){
	sfnt_builder_entry *item;
	HASH_FIND_INT(builder->tables, &tag, item);
	if(!item){
		item = createSegment(tag, buffer);
		HASH_ADD_INT(builder->tables, tag, item);
	} else {
		buffree(buffer);
	}
}
