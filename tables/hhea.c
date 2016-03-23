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

void caryll_hhea_to_json(caryll_font *font, JSON_Object *root) {
	if (!font->hhea) return;
	json_object_dotset_number(root, "hhea.version", font->hhea->version);
	json_object_dotset_number(root, "hhea.ascender", font->hhea->ascender);
	json_object_dotset_number(root, "hhea.descender", font->hhea->descender);
	json_object_dotset_number(root, "hhea.lineGap", font->hhea->lineGap);
	json_object_dotset_number(root, "hhea.advanceWithMax", font->hhea->advanceWithMax);
	json_object_dotset_number(root, "hhea.minLeftSideBearing", font->hhea->minLeftSideBearing);
	json_object_dotset_number(root, "hhea.minRightSideBearing", font->hhea->minRightSideBearing);
	json_object_dotset_number(root, "hhea.xMaxExtent", font->hhea->xMaxExtent);
	json_object_dotset_number(root, "hhea.caretSlopeRise", font->hhea->caretSlopeRise);
	json_object_dotset_number(root, "hhea.yMcaretSlopeRunax", font->hhea->caretSlopeRun);
	json_object_dotset_number(root, "hhea.caretOffset", font->hhea->caretOffset);
	json_object_dotset_number(root, "hhea.lowestmetricDataFormatRecPPEM", font->hhea->metricDataFormat);
	json_object_dotset_number(root, "hhea.numberOfMetrics", font->hhea->numberOfMetrics);
}
