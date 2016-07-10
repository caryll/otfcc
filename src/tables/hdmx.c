#include "hdmx.h"

table_hdmx *caryll_read_hdmx(caryll_packet packet, table_maxp *maxp) {
	FOR_TABLE('hdmx', table) {
		font_file_pointer data = table.data;

		table_hdmx *hdmx = (table_hdmx *)malloc(sizeof(table_hdmx) * 1);
		hdmx->version = read_16u(data);
		hdmx->numRecords = read_16u(data + 2);
		hdmx->sizeDeviceRecord = read_32u(data + 4);
		hdmx->records = (device_record *)malloc(sizeof(device_record) * hdmx->numRecords);

		for (uint32_t i = 0; i < hdmx->numRecords; i++) {
			hdmx->records[i].pixelSize = *(data + 8 + i * (2 + maxp->numGlyphs));
			hdmx->records[i].maxWidth = *(data + 8 + i * (2 + maxp->numGlyphs) + 1);
			hdmx->records[i].widths = (uint8_t *)malloc(sizeof(uint8_t) * maxp->numGlyphs);
			memcpy(hdmx->records[i].widths, data + 8 + i * (2 + maxp->numGlyphs) + 2,
			       maxp->numGlyphs);
		}

		return hdmx;
	}
	return NULL;
}

void caryll_delete_hdmx(table_hdmx *table) {
	if (table->records != NULL) {
		for (uint32_t i = 0; i < table->numRecords; i++) {
			if (table->records[i].widths != NULL) free(table->records[i].widths);
		}
		free(table->records);
	}
	free(table);
}
