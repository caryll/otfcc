#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "extern/json-builder.h"
#include "support/stopwatch.h"

#include "caryll-sfnt.h"
#include "caryll-font.h"

int main(int argc, char *argv[]) {
	struct timespec begin;
	time_now(&begin);

	caryll_sfnt *sfnt = caryll_sfnt_open(argv[1]);
	caryll_font *font = caryll_font_open(sfnt, 0);

	push_stopwatch("Parse SFNT", &begin);

	json_value *root = json_object_new(12);
	
	caryll_head_to_json(font->head, root);
	caryll_hhea_to_json(font->hhea, root);
	caryll_maxp_to_json(font->maxp, root);
	caryll_name_to_json(font->name, root);
	caryll_post_to_json(font->post, root);
	caryll_OS_2_to_json(font->OS_2, root);
	caryll_cmap_to_json(font->cmap, root);
	caryll_glyf_to_json(font->glyf, root);
	
	push_stopwatch("Convert to JSON", &begin);

	json_serialize_opts options;
	options.mode = json_serialize_mode_packed;
	options.opts = 0;
	options.indent_size = 4;

	if (isatty(fileno(stdout))) { options.mode = json_serialize_mode_multiline; }

	char *buf = malloc(json_measure_ex(root, options));
	json_serialize_ex(buf, root, options);
	push_stopwatch("Serialize to string", &begin);
	fputs(buf, stdout);
	push_stopwatch("Write to file", &begin);
	free(buf);
	json_value_free(root);

	caryll_font_close(font);
	caryll_sfnt_close(sfnt);
	return 0;
}
