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
		font->post = post;
	}
}
