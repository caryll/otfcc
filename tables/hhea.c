#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_hhea(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('hhea', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length < 36) {
			fprintf(stderr, "table 'hhea' corrupted.\n");
			font->hhea = NULL;
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
			font->hhea = hhea;
		}
	}
}

void caryll_hhea_to_json(caryll_font *font, json_value *root) {
	if (!font->hhea) return;
	json_value *hhea = json_object_new(13);
	json_object_push(hhea, "version", json_integer_new(font->hhea->version));
	json_object_push(hhea, "ascender", json_integer_new(font->hhea->ascender));
	json_object_push(hhea, "descender", json_integer_new(font->hhea->descender));
	json_object_push(hhea, "lineGap", json_integer_new(font->hhea->lineGap));
	json_object_push(hhea, "advanceWithMax", json_integer_new(font->hhea->advanceWithMax));
	json_object_push(hhea, "minLeftSideBearing", json_integer_new(font->hhea->minLeftSideBearing));
	json_object_push(hhea, "minRightSideBearing", json_integer_new(font->hhea->minRightSideBearing));
	json_object_push(hhea, "xMaxExtent", json_integer_new(font->hhea->xMaxExtent));
	json_object_push(hhea, "caretSlopeRise", json_integer_new(font->hhea->caretSlopeRise));
	json_object_push(hhea, "yMcaretSlopeRunax", json_integer_new(font->hhea->caretSlopeRun));
	json_object_push(hhea, "caretOffset", json_integer_new(font->hhea->caretOffset));
	json_object_push(hhea, "lowestmetricDataFormatRecPPEM", json_integer_new(font->hhea->metricDataFormat));
	json_object_push(hhea, "numberOfMetrics", json_integer_new(font->hhea->numberOfMetrics));
	json_object_push(root, "hhea", hhea);
}

