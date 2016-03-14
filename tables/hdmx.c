#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_hdmx(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('hdmx', table) {
		font_file_pointer data = table.data;

		table_hdmx *hdmx = (table_hdmx *)malloc(sizeof(table_hdmx) * 1);
		hdmx->version = caryll_blt16u(data);
		hdmx->numRecords = caryll_blt16u(data + 2);
		hdmx->sizeDeviceRecord = caryll_blt32u(data + 4);
		hdmx->records = (device_record *)malloc(sizeof(device_record) * hdmx->numRecords);

		for (uint32_t i = 0; i < hdmx->numRecords; i++) {
			hdmx->records[i].pixelSize = *(data + 8 + i * (2 + font->maxp->numGlyphs));
			hdmx->records[i].maxWidth = *(data + 8 + i * (2 + font->maxp->numGlyphs) + 1);
			hdmx->records[i].widths = (uint8_t *)malloc(sizeof(uint8_t) * font->maxp->numGlyphs);
			memcpy(hdmx->records[i].widths, data + 8 + i * (2 + font->maxp->numGlyphs) + 2, font->maxp->numGlyphs);
		}

		font->hdmx = hdmx;
	}
}
