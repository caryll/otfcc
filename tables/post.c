#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_post(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('post', table) {
		font_file_pointer data = table.data;

		table_post *post = (table_post *)malloc(sizeof(table_post) * 1);
		post->version = caryll_blt32u(data);
		post->italicAngle = caryll_blt32u(data + 4);
		post->underlinePosition = caryll_blt16u(data + 8);
		post->underlineThickness = caryll_blt16u(data + 10);
		post->isFixedPitch = caryll_blt32u(data + 12);
		post->minMemType42 = caryll_blt32u(data + 16);
		post->maxMemType42 = caryll_blt32u(data + 20);
		post->minMemType1 = caryll_blt32u(data + 24);
		post->maxMemType1 = caryll_blt32u(data + 28);
		post->post_name_map = NULL;
		// Foamt 2 additional glyph names
		if(post->version == 0x20000) {
			glyph_order_hash *map = malloc(sizeof(glyph_order_hash));
			*map = NULL;
			
			sds pendingNames[0x10000];
			memset(pendingNames, 0, sizeof(pendingNames));
			uint16_t numberGlyphs = caryll_blt16u(data + 32);
			uint32_t offset = 34 + 2 * numberGlyphs;
			uint16_t pendingNameIndex = 0;
			while(pendingNameIndex <= 0xFFFF && offset < table.length) {
				uint8_t len = data[offset];
				sds s;
				if(len > 0) {
					s = sdsnewlen(data + offset + 1, len);
				} else {
					s = sdsempty();
				}
				offset += len + 1;
				pendingNames[pendingNameIndex] = s;
				pendingNameIndex += 1;
			}
			for(uint16_t j = 0; j < numberGlyphs; j++){
				uint16_t nameMap = caryll_blt16u(data + 34 + 2 * j);
				if(nameMap > 258) {
					try_name_glyph(map, j, sdsdup(pendingNames[nameMap - 258]));
				}
			}
			for(uint32_t j = 0; j < pendingNameIndex; j++){
				sdsfree(pendingNames[j]);
			}
			post->post_name_map = map;
		}
		font->post = post;
	}
}
