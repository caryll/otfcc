#ifndef CARYLL_TABLES_VORG_H
#define CARYLL_TABLES_VORG_H

#include <font/caryll-sfnt.h>
#include <support/util.h>

typedef struct {
	int16_t verticalOrigin;
	uint16_t gid;
} VORG_entry;

typedef struct {
	VORG_entry *entries;
	uint16_t numVertOriginYMetrics;
	int16_t defaultVerticalOrigin;
} table_VORG;

table_VORG *caryll_read_VORG(caryll_packet packet);
void caryll_delete_VORG(table_VORG *vorg);
#endif
