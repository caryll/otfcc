#ifndef CARYLL_INCLUDE_TABLE_HHEA_H
#define CARYLL_INCLUDE_TABLE_HHEA_H

#include "table-common.h"

typedef struct {
	// Horizontal header
	f16dot16 version;
	int16_t ascender;
	int16_t descender;
	int16_t lineGap;
	uint16_t advanceWithMax;
	int16_t minLeftSideBearing;
	int16_t minRightSideBearing;
	int16_t xMaxExtent;
	int16_t caretSlopeRise;
	int16_t caretSlopeRun;
	int16_t caretOffset;
	int16_t reserved[4];
	int16_t metricDataFormat;
	uint16_t numberOfMetrics;
} table_hhea;

table_hhea *otfcc_newTablehhea();
table_hhea *otfcc_readTablehhea(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpTablehhea(const table_hhea *table, json_value *root, const otfcc_Options *options);
table_hhea *otfcc_parseTablehhea(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildTablehhea(const table_hhea *hhea, const otfcc_Options *options);

#endif
