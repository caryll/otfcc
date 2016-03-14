#ifndef CARYLL_TABLES_HDMX_H
#define CARYLL_TABLES_HDMX_H

#include <stdint.h>
#include "../caryll-font.h"

typedef struct {
	uint8_t pixelSize;
	uint8_t maxWidth;
	uint8_t *widths;
} device_record;

typedef struct {
	// Horizontal device metrics
	uint16_t version;
	int16_t numRecords;
	int32_t sizeDeviceRecord;
	device_record *records;
} table_hdmx;

void caryll_read_hdmx(caryll_font *font, caryll_packet packet);

#endif
