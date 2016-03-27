#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "extern/json.h"
#include "support/stopwatch.h"

#include "caryll-sfnt.h"
#include "caryll-font.h"

int main(int argc, char *argv[]) {
	struct timespec begin;
	time_now(&begin);

	char *buffer = 0;
	long length;
	FILE *f = fopen(argv[1], "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) { fread(buffer, 1, length, f); }
		fclose(f);
	}
	push_stopwatch("Read file", &begin);
	if (buffer) {
		json_value *root = json_parse(buffer, length);
		free(buffer);
		push_stopwatch("Parse JSON", &begin);
		if (root) {
			caryll_font *font = caryll_font_new();
			font->glyph_order = caryll_glyphorder_from_json(root);
			font->cmap = caryll_cmap_from_json(root);
			font->glyf = caryll_glyf_from_json(root, *font->glyph_order);
			
			caryll_font_consolidate(font);
			
			push_stopwatch("Parse json to font", &begin);
			json_value_free(root);
			caryll_font_close(font);
		}
	}
	return 0;
}
