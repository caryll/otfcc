#include "vhea.h"

table_vhea *caryll_read_vhea(caryll_packet packet) {
	FOR_TABLE('vhea', table) {
		font_file_pointer data = table.data;
		size_t length = table.length;
		if (length >= 30) {
			table_vhea *vhea = (table_vhea *)malloc(sizeof(table_vhea) * 1);
			vhea->version = caryll_blt32u(data);
			vhea->ascent = caryll_blt16u(data + 4);
			vhea->descent = caryll_blt16u(data + 6);
			vhea->advanceHeightMax = caryll_blt16u(data + 8);
			vhea->minTop = caryll_blt16u(data + 10);
			vhea->minBottom = caryll_blt16u(data + 12);
			vhea->yMaxExtent = caryll_blt16u(data + 14);
			vhea->caretSlopeRise = caryll_blt16u(data + 16);
			vhea->caretSlopeRun = caryll_blt16u(data + 18);
			vhea->caretOffset = caryll_blt16u(data + 20);
			vhea->dummy[0] = *(data + 22);
			vhea->dummy[1] = *(data + 23);
			vhea->dummy[2] = *(data + 24);
			vhea->dummy[3] = *(data + 24);
			vhea->metricDataFormat = caryll_blt16u(data + 26);
			vhea->numOf = caryll_blt16u(data + 28);

			return vhea;
		}
	}
	return NULL;
}
