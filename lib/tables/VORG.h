#ifndef CARYLL_TABLES_VORG_H
#define CARYLL_TABLES_VORG_H

#include <font/caryll-sfnt.h>
#include <support/util.h>

typedef struct {
	uint16_t gid;
	int16_t verticalOrigin;
} VORG_entry;

typedef struct {
	uint16_t numVertOriginYMetrics;
	int16_t defaultVerticalOrigin;
	VORG_entry *entries;
} table_VORG;

void caryll_delete_VORG(table_VORG *vorg);
table_VORG *caryll_read_VORG(caryll_packet packet);
caryll_buffer *caryll_write_VORG(table_VORG *table, const caryll_options *options);
#endif
