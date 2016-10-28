#ifndef CARYLL_INCLUDE_TABLE_HDMX_H
#define CARYLL_INCLUDE_TABLE_HDMX_H

#include "table-common.h"
#include "maxp.h"

typedef struct {
	uint8_t pixelSize;
	uint8_t maxWidth;
	uint8_t *widths;
} device_record;

typedef struct {
	// Horizontal device metrics
	uint16_t version;
	uint16_t numRecords;
	uint32_t sizeDeviceRecord;
	device_record *records;
} table_hdmx;

table_hdmx *otfcc_readHdmx(otfcc_Packet packet, const otfcc_Options *options, table_maxp *maxp);
void otfcc_deleteHdmx(table_hdmx *table);

#endif
