#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_head(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('head', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length < 54) {
			printf("table 'head' corrupted.\n");
			font->head = NULL;
		} else {
			table_head *head = (table_head *)malloc(sizeof(table_head) * 1);
			head->version = caryll_blt32u(data);
			head->fontRevison = caryll_blt32u(data + 4);
			head->checkSumAdjustment = caryll_blt32u(data + 8);
			head->magicNumber = caryll_blt32u(data + 12);
			head->flags = caryll_blt16u(data + 16);
			head->unitsPerEm = caryll_blt16u(data + 18);
			head->created = caryll_blt64u(data + 20);
			head->modified = caryll_blt64u(data + 28);
			head->xMin = caryll_blt16u(data + 36);
			head->yMin = caryll_blt16u(data + 38);
			head->xMax = caryll_blt16u(data + 40);
			head->yMax = caryll_blt16u(data + 42);
			head->macStyle = caryll_blt16u(data + 44);
			head->lowestRecPPEM = caryll_blt16u(data + 46);
			head->fontDirectoryHint = caryll_blt16u(data + 48);
			head->indexToLocFormat = caryll_blt16u(data + 50);
			head->glyphDataFormat = caryll_blt16u(data + 52);
			font->head = head;
		}
	}
}
