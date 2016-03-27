#include "hhea.h"

table_hhea *caryll_read_hhea(caryll_packet packet) {
	FOR_TABLE('hhea', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length < 36) {
			fprintf(stderr, "table 'hhea' corrupted.\n");
		} else {
			table_hhea *hhea = (table_hhea *)malloc(sizeof(table_hhea) * 1);
			hhea->version = caryll_blt32u(data);
			hhea->ascender = caryll_blt16u(data + 4);
			hhea->descender = caryll_blt16u(data + 6);
			hhea->lineGap = caryll_blt16u(data + 8);
			hhea->advanceWithMax = caryll_blt16u(data + 10);
			hhea->minLeftSideBearing = caryll_blt16u(data + 12);
			hhea->minRightSideBearing = caryll_blt16u(data + 14);
			hhea->xMaxExtent = caryll_blt16u(data + 16);
			hhea->caretSlopeRise = caryll_blt16u(data + 18);
			hhea->caretSlopeRun = caryll_blt16u(data + 20);
			hhea->caretOffset = caryll_blt16u(data + 22);
			hhea->reserved[0] = caryll_blt16u(data + 24);
			hhea->reserved[1] = caryll_blt16u(data + 26);
			hhea->reserved[2] = caryll_blt16u(data + 28);
			hhea->reserved[3] = caryll_blt16u(data + 30);
			hhea->metricDataFormat = caryll_blt16u(data + 32);
			hhea->numberOfMetrics = caryll_blt16u(data + 34);
			return hhea;
		}
	}
	return NULL;
}

void caryll_hhea_to_json(table_hhea *table, json_value *root, caryll_dump_options dumpopts) {
	if (!table) return;
	json_value *hhea = json_object_new(13);
	json_object_push(hhea, "version", json_integer_new(table->version));
	json_object_push(hhea, "ascender", json_integer_new(table->ascender));
	json_object_push(hhea, "descender", json_integer_new(table->descender));
	json_object_push(hhea, "lineGap", json_integer_new(table->lineGap));
	json_object_push(hhea, "advanceWithMax", json_integer_new(table->advanceWithMax));
	json_object_push(hhea, "minLeftSideBearing", json_integer_new(table->minLeftSideBearing));
	json_object_push(hhea, "minRightSideBearing", json_integer_new(table->minRightSideBearing));
	json_object_push(hhea, "xMaxExtent", json_integer_new(table->xMaxExtent));
	json_object_push(hhea, "caretSlopeRise", json_integer_new(table->caretSlopeRise));
	json_object_push(hhea, "yMcaretSlopeRunax", json_integer_new(table->caretSlopeRun));
	json_object_push(hhea, "caretOffset", json_integer_new(table->caretOffset));
	json_object_push(hhea, "lowestmetricDataFormatRecPPEM", json_integer_new(table->metricDataFormat));
	json_object_push(hhea, "numberOfMetrics", json_integer_new(table->numberOfMetrics));
	json_object_push(root, "hhea", hhea);
}
